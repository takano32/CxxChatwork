# CxxChatwork

CxxChatwork は、C++23 で実装した小さな Slack Outgoing Webhook 受信サーバーです。
Slack の Outgoing Webhook（`application/x-www-form-urlencoded`）を受け取り、メッセージ本文に含まれる URL を抽出してはてなブックマークに追加・更新します。

抽出した URL は `HatenaBookmarkClient` がはてなブックマークに登録し、送信者の `user_id` をタグとして付与します。

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
| `SLACK_OUTGOING_TOKEN` | ✓ | Slack Outgoing Webhook の Token。受信リクエストの `token` フィールドと照合します |

`.env.example` に設定例があります。`SLACK_OUTGOING_TOKEN` は Slack の Outgoing Webhook 設定画面に表示される Token を設定してください。

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

## Slack Outgoing Webhook の受信内容

Slack の Outgoing Webhook は `application/x-www-form-urlencoded` で `POST /cxx_chatwork` に送られてきます（このパス以外への POST は `404 Not Found`）。`SlackPayload` がコンストラクタで本文を form としてパースし、各フィールドを URL デコードして保持します。利用するフィールドは次のとおりです。

| フィールド | 用途 |
| --- | --- |
| `token` | `SLACK_OUTGOING_TOKEN` と照合してリクエストを検証します。不一致なら `401 Unauthorized` を返して何もしません。 |
| `text` | URL を抽出する対象のメッセージ本文です。 |
| `user_id` | 送信者。ブックマークのタグとして付与します。 |

`text` から `URI::extract_url()` で URL を抽出します（対象は `http` / `https` のみ）。

抽出した各 URL について、まず `get_bookmark()` で既存のブックマークを取得し、その `comment` と `tags` を引き継ぎます（既存のコメント・タグが上書きで消えるのを防ぎます。まだブックマークされていない URL は取得に失敗するため新規登録になります）。次に、送信者の `user_id` を `tags` に追加してから `post_bookmark()` を呼び出します。`user_id` が空の場合や、すでに同じタグが含まれている場合は追加しません。

なお、はてな API の**レスポンス**は JSON のため、`JSON` クラス（`JSON::parse`）はその解析に引き続き使用します。

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

タグははてなの仕様に合わせ、コメント先頭に `[tag1][tag2]本文` の記法で埋め込んで送信します。`get_bookmark()` で取得する際は、`comment`（タグ記法を除いた本文）と `tags`（配列）に分解して返します。取得したタグ要素に空白が含まれる場合は個別のタグに分割します。

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
[ADD]: [] https://example.com/docs #
[UPDATE]: [tech][cpp] https://github.com/takano32/brevaluck/pulls # 参考資料
[GET]: [tech][cpp] https://example.com # 参考資料
```

形式は `[メソッド]: [タグ1][タグ2] URL # コメント` です（タグが無い場合は `[]`）。

## 動作確認

別のターミナルでサーバーを起動します。テスト用の `token` は `TOKEN` 変数で揃えます（既定値 `test-token`）。

```sh
make run PORT=48080 TOKEN=test-token
```

Outgoing Webhook の受信を確認するには、次を実行します（`TOKEN` は `run` と一致させます）。

```sh
make test-webhook PORT=48080 TOKEN=test-token
make test-uri PORT=48080 TOKEN=test-token
```

`token` が一致しない場合は `401 Unauthorized` が返ります。

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
| `make run PORT=48080 TOKEN=test-token` | 環境変数を指定してサーバーを起動します。 |
| `make clean` | `build/` を削除します。 |
| `make rebuild` | `clean` の後に `build` を実行します。 |
| `make test-webhook PORT=48080 TOKEN=test-token` | 起動中のサーバーへ URL を含む Outgoing Webhook payload を POST します。 |
| `make test-uri PORT=48080 TOKEN=test-token` | 起動中のサーバーへ複数の URL を含む Outgoing Webhook payload を POST します。 |
| `make test-get PORT=48080` | 起動中のサーバーへ `GET /bookmark?url=...` を送り、ブックマーク情報を取得します。 |
| `make install-service` | systemd ユニットを生成・設置して reload します（`sudo` を使用）。 |
| `make uninstall-service` | systemd ユニットを停止・無効化して削除します（`sudo` を使用）。 |
| `make service-status` | systemd サービスの状態を表示します。 |
| `make service-logs` | systemd サービスのログを追従表示します。 |

## Makefile の変数

| 変数 | 初期値 | 説明 |
| --- | --- | --- |
| `PORT` | `48080` | `make run` とテスト用ターゲットで使うポート番号です。 |
| `LISTEN` | (空、`0.0.0.0` 相当) | `make run` で使うリッスンアドレスです。 |
| `TOKEN` | `test-token` | `make run` とテスト用ターゲットで使う Outgoing Webhook の token です。 |
| `BUILD_DIR` | `build` | CMake のビルドディレクトリです。 |

## 終了方法

サーバーはフォアグラウンドで動作します。
終了するには `Ctrl-C` を押してください。

## systemd でのデプロイ

サーバーを常駐させ、`systemctl` で制御するための systemd ユニットを `deploy/cxx-chatwork.service` に用意しています。`make install-service` が、その時点の**実行ユーザー**と**作業ディレクトリ**を埋め込んで `/etc/systemd/system/cxx-chatwork.service` を生成します。

### 事前準備

1. ビルド済みであること（`./build/cxx_chatwork` が存在）。無ければ `make build` を実行します。
2. `.env` を作業ディレクトリに配置していること。systemd の `EnvironmentFile` として読み込まれます。
   - **`HATENA_ACCESS_TOKEN` / `HATENA_ACCESS_TOKEN_SECRET` を必ず設定してください。** これらが無いと起動時に OAuth の PIN 入力を待ちますが、systemd 配下では標準入力が無いため起動に失敗します。
   - **`SLACK_OUTGOING_TOKEN` を必ず設定してください。** Slack の Outgoing Webhook 設定画面に表示される Token です。未設定だと起動に失敗します。
   - 外部（Slack）から受信する場合は `CXX_CHATWORK_LISTEN=0.0.0.0` とし、ファイアウォール／セキュリティグループで `CXX_CHATWORK_PORT` を開放します。リバースプロキシ（Caddy など）を前段に置く場合は `CXX_CHATWORK_LISTEN=127.0.0.1` にしてプロキシ経由のみ公開するのが安全です。

### 設置と起動

```sh
cd ~/CxxChatwork
make build              # まだビルドしていなければ
make install-service    # ユニットを生成・設置（sudo）
sudo systemctl enable --now cxx-chatwork
```

### 運用

```sh
make service-status                 # 状態確認（systemctl status cxx-chatwork）
make service-logs                   # ログ追従（journalctl -u cxx-chatwork -f）
sudo systemctl restart cxx-chatwork # 再起動
```

更新を反映するには、リポジトリを更新してビルドし直し、サービスを再起動します。

```sh
git pull
make build
sudo systemctl restart cxx-chatwork
```

撤去する場合は `make uninstall-service` を実行します。

## Caddy で HTTPS 公開

Slack の Outgoing Webhook は HTTPS の URL を要求します。[Caddy](https://caddyserver.com/) をリバースプロキシとして前段に置くと、Let's Encrypt の証明書取得・更新まで自動で行われます。本サーバーは `CXX_CHATWORK_LISTEN=127.0.0.1` でローカルのみ待ち受けにし、外部公開は Caddy(443) 経由のみとするのが安全です。

### 前提

- **ドメイン名**が必要です。独自ドメインの A レコードをサーバーの外部 IP に向けます。ドメインが無い場合は `<IPをハイフン区切り>.sslip.io`（例 `203-0-113-10.sslip.io`）が DNS 設定なしで IP に解決されるので利用できます。
- 証明書取得（ACME チャレンジ）のため **80 / 443 番ポートを開放**します。アプリのポート（`CXX_CHATWORK_PORT`）は外部に開ける必要はありません。

### インストールと設定

Caddy を公式 apt リポジトリから入れます（Debian / Ubuntu）。

```sh
sudo apt-get install -y debian-keyring debian-archive-keyring apt-transport-https curl gnupg
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' \
  | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' \
  | sudo tee /etc/apt/sources.list.d/caddy-stable.list
sudo apt-get update && sudo apt-get install -y caddy
```

`/etc/caddy/Caddyfile` を次の内容にします（`<ドメイン>` は実際のホスト名に置き換え）。webhook の `/cxx_chatwork` だけをアプリへ転送し、それ以外のパスは 404 を返します。

```caddyfile
<ドメイン> {
	handle /cxx_chatwork {
		reverse_proxy localhost:48080
	}
	handle {
		respond 404
	}
}
```

設定を検証して反映します。

```sh
sudo caddy validate --config /etc/caddy/Caddyfile
sudo systemctl reload caddy
```

### 自動 HTTPS について

Caddyfile のサイトアドレスに実ドメイン名を書くだけで、Caddy の自動 HTTPS が有効になります。Let's Encrypt 固有の設定は不要で、Caddy が起動時に ACME 経由で証明書を取得し、期限が近づくと自動更新します。取得した証明書とアカウント情報は `/var/lib/caddy/.local/share/caddy/` 配下に保存されます。

### Slack 側の設定

Slack の Outgoing Webhook の URL に `https://<ドメイン>/cxx_chatwork` を設定します。
