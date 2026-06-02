# CxxChatwork

CxxChatwork は、C++23 で実装した小さな Slack Webhook 受信サーバーです。
Slack から HTTP POST で送られてきた JSON payload を受け取り、メッセージ本文に含まれる URL を登録済みのクライアントで処理します。

標準では `HatenaBookmarkClient` が有効になっており、抽出した URL をはてなブックマークに追加・更新します。

## 必要なもの

- C++23 に対応した C++ コンパイラ
- CMake 3.20 以上
- Make
- OpenSSL (libcrypto) — はてな OAuth の HMAC-SHA1 署名に使用
- libcurl — はてな API への HTTPS リクエストに使用
- 動作確認用に `curl`

## ビルド方法

```sh
make build
```

このコマンドは内部で以下を実行します。

```sh
cmake -S . -B build
cmake --build build
```

実行ファイルは `build/cxx_chatwork` に生成されます。

## 起動方法

起動前に以下の環境変数を設定してください。

| 環境変数 | 必須 | 説明 |
| --- | --- | --- |
| `CXX_CHATWORK_PORT` | ✓ | サーバーの待受ポート番号 (1–65535) |
| `CXX_CHATWORK_LISTEN` | — | 待受アドレス。省略時は `0.0.0.0` |
| `HATENA_CONSUMER_KEY` | ✓ | はてな OAuth の consumer key |
| `HATENA_CONSUMER_SECRET` | ✓ | はてな OAuth の consumer secret |
| `HATENA_ACCESS_TOKEN` | — | はてな OAuth のアクセストークン。設定すると OAuth フローを省略します |
| `HATENA_ACCESS_TOKEN_SECRET` | — | アクセストークンシークレット。`HATENA_ACCESS_TOKEN` と対で指定します |

`.env.example` に設定例があります。

`HATENA_ACCESS_TOKEN` が未設定の場合、起動時に OAuth 認証フローが始まります。表示された URL をブラウザで開いてアプリを認可し、発行された PIN を入力してください。初回認証で得られたトークンが標準出力に表示されるので、それを `HATENA_ACCESS_TOKEN` / `HATENA_ACCESS_TOKEN_SECRET` に設定すれば、次回以降は PIN 入力を省略できます。

```sh
make run PORT=48080
# => Authorize at: https://www.hatena.com/oauth/authorize?oauth_token=...
# => Enter PIN: xxxxxx
# => HATENA_ACCESS_TOKEN=...
# => HATENA_ACCESS_TOKEN_SECRET=...
# => Listening on 0.0.0.0:48080
```

直接実行する場合は次のように指定します。

```sh
CXX_CHATWORK_PORT=48080 ./build/cxx_chatwork
CXX_CHATWORK_PORT=48080 CXX_CHATWORK_LISTEN=127.0.0.1 ./build/cxx_chatwork
```

`CXX_CHATWORK_PORT` が未指定・空文字・範囲外、または `CXX_CHATWORK_LISTEN` に無効な IPv4 アドレスを指定した場合はエラーで終了します。

## Slack Webhook の受信内容

サーバーは HTTP POST の本文を Slack の JSON payload として扱います。JSON のパースは自前の `JSON` クラス（`JSON::parse`）で行い、`SlackPayload` がコンストラクタでメッセージ本文と送信者を解決して保持します。

メッセージ本文（`text`）は次の順番で探します。

1. `event.text`
2. `message.text`
3. トップレベルの `text`

いずれも見つからない場合は、本文（body）全体をメッセージ本文として扱います。

送信者（`user_id`）も同様に `event.user` → `message.user` → トップレベル `user` の順で探します。

見つかったテキストから `URI::extract_url()` で URL を抽出します。抽出対象は `http` / `https` スキームの URL のみです。

抽出した各 URL について、まず `get_bookmark()` で既存のブックマークを取得し、その `comment` と `tags` を引き継ぎます（既存のコメント・タグが上書きで消えるのを防ぎます。まだブックマークされていない URL は取得に失敗するため新規登録になります）。次に、送信者の `user_id` を `tags` に追加してから `post_bookmark()` を呼び出します。`user_id` が空の場合や、すでに同じタグが含まれている場合は追加しません。

## Slack URL verification

Slack Events API の URL verification payload に含まれる `challenge` に対応しています。

次のような payload を受け取った場合:

```json
{
  "type": "url_verification",
  "challenge": "challenge-token"
}
```

HTTP レスポンス本文として `challenge-token` を返します。

## ブックマーク取得エンドポイント

`GET /bookmark?url=...` で、指定した URL のブックマーク情報を取得できます。サーバーは `HatenaBookmarkClient::get_bookmark()` を呼び出し、レスポンス本文に `[タグ1][タグ2], URL, コメント` の形式で結果を返します。

```sh
curl -sS -G http://127.0.0.1:48080/bookmark \
  --data-urlencode 'url=https://github.com/takano32/brevaluck/'
```

## はてなブックマーク連携

### クラス構成

```
HatenaClient           (OAuth を保持するベースクラス)
  └─ HatenaBookmarkClient  (post_bookmark / get_bookmark を実装)

HatenaBookmark         (url, comment, tags を保持する値クラス)
```

`Server` は `HatenaBookmarkClient` を1つ保持し、抽出した URL ごとに `get_bookmark()` と `post_bookmark()` を直接呼び出します。

### OAuth 認証

`OAuth` クラスが認証を担当します。`consumer_key` と `consumer_secret` はコンストラクタ引数で渡し、環境変数の読み取りは呼び出し側の責務です。

```cpp
chatwork::OAuth oauth(consumer_key, consumer_secret);
oauth.authorize();
// => Authorize at: https://www.hatena.com/oauth/authorize?oauth_token=...
// => Enter PIN:
```

`authorize()` の処理順:
1. はてな OAuth initiate エンドポイントへ POST → リクエストトークン取得
2. 認可 URL を標準出力に表示
3. 標準入力から PIN を受け取る
4. アクセストークンに交換して内部に保持

### ブックマークの追加・更新・取得

`HatenaBookmarkClient` の `post_bookmark(url, comment, tags)` でブックマークを追加または更新し、`get_bookmark(url)` でブックマーク情報を取得します。`comment` と `tags` は省略できます。

いずれも戻り値は `std::tuple<HatenaBookmark, HatenaBookmarkResult>` です。`HatenaBookmarkResult` は `Added`（HTTP 201）/ `Updated`（HTTP 200）/ `Got`（取得）の3値です。

```cpp
chatwork::OAuth oauth(consumer_key, consumer_secret);
oauth.authorize();
chatwork::HatenaBookmarkClient client(std::move(oauth));

// URL のみ
auto [bookmark, result] = client.post_bookmark("https://example.com");

// コメント付き
client.post_bookmark("https://example.com", "参考資料");

// コメントとタグ付き
client.post_bookmark("https://example.com", "参考資料", {"tech", "cpp"});

// 取得
auto [got, _] = client.get_bookmark("https://example.com");
// got.url(), got.comment(), got.tags()
```

ブックマークが追加・更新・取得されると、標準出力に次の形式で表示されます。

```text
[ADD] [], https://example.com/docs,
[UPDATE] [tech][cpp], https://github.com/takano32/brevaluck/pulls, 参考資料
[GET] [tech][cpp], https://example.com, 参考資料
```

形式は `[ラベル] [タグ1][タグ2], URL, コメント` です。

## 動作確認

別のターミナルでサーバーを起動します。

```sh
make run PORT=48080
```

メッセージ受信を確認するには、次を実行します。

```sh
make test-webhook PORT=48080
```

URL verification の応答を確認するには、次を実行します。

```sh
make test-challenge PORT=48080
```

レスポンス本文に `challenge-token` が返れば成功です。

ブックマーク取得を確認するには、次を実行します。

```sh
make test-get PORT=48080
```

## Makefile のターゲット

| ターゲット | 説明 |
| --- | --- |
| `make help` | 利用できるターゲットと変数を表示します。 |
| `make configure` | CMake のビルドファイルを `build/` に生成します。 |
| `make build` | `configure` を実行してから実行ファイルをビルドします。 |
| `make run PORT=48080` | `CXX_CHATWORK_PORT=48080` を指定してサーバーを起動します。 |
| `make clean` | `build/` を削除します。 |
| `make rebuild` | `clean` の後に `build` を実行します。 |
| `make test-webhook PORT=48080` | 起動中のサーバーへ URL を含む Slack メッセージ payload を POST します。 |
| `make test-challenge PORT=48080` | 起動中のサーバーへ Slack URL verification payload を POST します。 |
| `make test-uri PORT=48080` | 起動中のサーバーへ複数の URL を含む Slack メッセージ payload を POST します。 |
| `make test-get PORT=48080` | 起動中のサーバーへ `GET /bookmark?url=...` を送り、ブックマーク情報を取得します。 |

## Makefile の変数

| 変数 | 初期値 | 説明 |
| --- | --- | --- |
| `PORT` | `48080` | `make run` とテスト用ターゲットで使うポート番号です。 |
| `LISTEN` | (空、`0.0.0.0` 相当) | `make run` で使うリッスンアドレスです。 |
| `BUILD_DIR` | `build` | CMake のビルドディレクトリです。 |

## 終了方法

サーバーはフォアグラウンドで動作します。
終了するには `Ctrl-C` を押してください。
