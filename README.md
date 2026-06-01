# CxxChatwork

CxxChatwork は、C++23 で実装した小さな Slack Webhook 受信サーバーです。
Slack から HTTP POST で送られてきた JSON payload を受け取り、メッセージ本文に含まれる URL を1行ずつ標準出力に表示します。

## 必要なもの

- C++23 に対応した C++ コンパイラ
- CMake 3.20 以上
- Make
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

使用するポート番号は環境変数 `CXX_CHATWORK_PORT` で指定します。

Makefile 経由で起動する場合は、`PORT` にポート番号を指定します。

```sh
make run PORT=8080
```

直接実行する場合は、次のように `CXX_CHATWORK_PORT` を指定します。

```sh
CXX_CHATWORK_PORT=8080 ./build/cxx_chatwork
```

起動に成功すると、次のように待受ポートが表示されます。

```text
Listening on port 8080
```

`CXX_CHATWORK_PORT` が未指定、空文字、または `1` から `65535` の範囲外の場合はエラーで終了します。

## Slack Webhook の受信内容

サーバーは HTTP POST の本文を Slack の JSON payload として扱います。
次の順番でメッセージ本文を探します。

1. `event.text`
2. `message.text`
3. トップレベルの `text`

見つかったテキストから URL を抽出し、1行につき1件ずつ標準出力に表示します。

たとえば次の payload を受け取ると:

```json
{
  "event": {
    "type": "message",
    "text": "参考: https://example.com/docs と https://github.com/org/repo を見てください。"
  }
}
```

標準出力に次のように表示します。

```text
https://example.com/docs
https://github.com/org/repo
```

レスポンスは通常 `200 OK` で、本文は `ok` です。

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

## 動作確認

別のターミナルでサーバーを起動します。

```sh
make run PORT=8080
```

メッセージ受信を確認するには、次を実行します。

```sh
make test-webhook PORT=8080
```

サーバー側の標準出力に次のように表示されます。

```text
https://github.com/org/repo/pull/1
```

URL verification の応答を確認するには、次を実行します。

```sh
make test-challenge PORT=8080
```

レスポンス本文に `challenge-token` が返れば成功です。

## Makefile のターゲット

| ターゲット | 説明 |
| --- | --- |
| `make help` | 利用できるターゲットと変数を表示します。 |
| `make configure` | CMake のビルドファイルを `build/` に生成します。 |
| `make build` | `configure` を実行してから実行ファイルをビルドします。 |
| `make run PORT=8080` | `CXX_CHATWORK_PORT=8080` を指定してサーバーを起動します。 |
| `make clean` | `build/` を削除します。 |
| `make rebuild` | `clean` の後に `build` を実行します。 |
| `make test-webhook PORT=8080` | 起動中のサーバーへ URL を含む Slack メッセージ payload を POST します。 |
| `make test-challenge PORT=8080` | 起動中のサーバーへ Slack URL verification payload を POST します。 |
| `make test-uri PORT=8080` | 起動中のサーバーへ複数の URL を含む Slack メッセージ payload を POST します。 |

## Makefile の変数

| 変数 | 初期値 | 説明 |
| --- | --- | --- |
| `PORT` | `8080` | `make run` とテスト用ターゲットで使うポート番号です。 |
| `BUILD_DIR` | `build` | CMake のビルドディレクトリです。 |

## 終了方法

サーバーはフォアグラウンドで動作します。
終了するには `Ctrl-C` を押してください。
