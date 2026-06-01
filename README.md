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

起動時に OAuth 認証フローが始まります。表示された URL をブラウザで開いてアプリを認可し、発行された PIN を入力してください。

```sh
make run PORT=48080
# => Authorize at: https://www.hatena.com/oauth/authorize?oauth_token=...
# => Enter PIN: xxxxxx
# => Listening on 0.0.0.0:48080
```

直接実行する場合は次のように指定します。

```sh
CXX_CHATWORK_PORT=48080 ./build/cxx_chatwork
CXX_CHATWORK_PORT=48080 CXX_CHATWORK_LISTEN=127.0.0.1 ./build/cxx_chatwork
```

`CXX_CHATWORK_PORT` が未指定・空文字・範囲外、または `CXX_CHATWORK_LISTEN` に無効な IPv4 アドレスを指定した場合はエラーで終了します。

## Slack Webhook の受信内容

サーバーは HTTP POST の本文を Slack の JSON payload として扱います。
次の順番でメッセージ本文を探します。

1. `event.text`
2. `message.text`
3. トップレベルの `text`

見つかったテキストから URL を抽出し、登録済みの全クライアントの `process()` を呼び出します。

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

## はてなブックマーク連携

### クラス構成

```
HatenaClient           (純粋仮想 process(uri) を持つベースクラス)
  └─ HatenaBookmarkClient  (post_bookmark を実装)
```

`Server` は `HatenaClient` の配列を持ち、抽出した URI ごとに全クライアントの `process()` を呼び出します。

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

### ブックマークの追加・更新

`HatenaBookmarkClient` の `post_bookmark(url, comment, tags)` でブックマークを追加または更新します。`comment` と `tags` は省略できます。

```cpp
chatwork::OAuth oauth(consumer_key, consumer_secret);
oauth.authorize();
chatwork::HatenaBookmarkClient client(std::move(oauth));

// URL のみ
client.post_bookmark("https://example.com");

// コメント付き
client.post_bookmark("https://example.com", "参考資料");

// コメントとタグ付き
client.post_bookmark("https://example.com", "参考資料", {"tech", "cpp"});
```

Webhook 経由でブックマークが追加・更新されると、標準出力に次のように表示されます。

```text
[ADD] https://example.com/docs
[UPDATE] https://github.com/takano32/brevaluck/pulls
```

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

## Makefile の変数

| 変数 | 初期値 | 説明 |
| --- | --- | --- |
| `PORT` | `48080` | `make run` とテスト用ターゲットで使うポート番号です。 |
| `LISTEN` | (空、`0.0.0.0` 相当) | `make run` で使うリッスンアドレスです。 |
| `BUILD_DIR` | `build` | CMake のビルドディレクトリです。 |

## 終了方法

サーバーはフォアグラウンドで動作します。
終了するには `Ctrl-C` を押してください。
