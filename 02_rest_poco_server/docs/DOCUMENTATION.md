# Расширенная документация Poco Template Server

## 1. Структура сервиса

### 1.1 Обзор проекта

```
poco_template_server/
├── src/
│   ├── main.cpp                 # Точка входа, ServerApplication, инициализация
│   └── handler/                 # HTTP-обработчики (handlers)
│       ├── router_factory.h     # Маршрутизация запросов по URI и методу
│       ├── request_counter.h    # Общие счётчики Prometheus (extern)
│       ├── auth_config.h        # Конфигурация JWT (JWT_SECRET)
│       ├── hello_world_handler.h
│       ├── helloworld_jwt_handler.h
│       ├── auth_handler.h
│       ├── swagger_handler.h
│       ├── metrics_handler.h
│       └── not_found_handler.h
├── loadtest/                    # Нагрузочное тестирование
│   ├── run.sh
│   └── README.md
├── postman/                     # Коллекция Postman для тестирования API
├── docs/
│   └── DOCUMENTATION.md         # Расширенная документация
├── CMakeLists.txt
├── Dockerfile                   # Multi-stage: builder + runner
├── docker-compose.yaml
└── README.md
```

### 1.2 Архитектура

Сервис построен на **POCO C++ Libraries** и использует:

- **Poco::Util::ServerApplication** — базовый класс приложения с поддержкой сигналов (SIGTERM, SIGINT)
- **Poco::Net::HTTPServer** — многопоточный HTTP-сервер
- **HTTPRequestHandlerFactory** — фабрика, создающая обработчик для каждого запроса
- **HTTPRequestHandler** — абстрактный класс обработчика с методом `handleRequest()`

Поток запроса:
1. `HTTPServer` принимает TCP-соединение
2. Парсит HTTP-запрос в `HTTPServerRequest`
3. Вызывает `RouterFactory::createRequestHandler(request)` — по URI и методу выбирается handler
4. Создаётся экземпляр handler'а, вызывается `handleRequest(request, response)`
5. Handler формирует ответ в `HTTPServerResponse`
6. После возврата из `handleRequest` объект handler уничтожается

### 1.3 Маршрутизация

`RouterFactory` наследует `HTTPRequestHandlerFactory` и реализует `createRequestHandler()`:

```cpp
HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) override {
    const std::string& uri = request.getURI();
    const std::string& method = request.getMethod();
    // Сопоставление URI + method → конкретный Handler
    if (uri == "/api/v1/helloworld" && method == "GET") return new HelloWorldHandler();
    // ...
    return new NotFoundHandler();  // Fallback
}
```

Каждый запрос обрабатывается **отдельным экземпляром** handler'а. Handler создаётся через `new` и удаляется сервером после обработки.

---

## 2. Принцип создания веб-сервисов на POCO

### 2.1 Минимальный handler

Любой handler наследует `Poco::Net::HTTPRequestHandler` и реализует `handleRequest()`:

```cpp
class MyHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentType("text/plain");
        std::ostream& ostr = response.send();
        ostr << "Hello";
    }
};
```

### 2.2 Типичный паттерн handler'а

1. **Измерение времени** — `Poco::Timestamp start` в начале
2. **Инкремент счётчика запросов** — `g_httpRequests->inc()`
3. **Обработка** — чтение `request`, формирование `response`
4. **Метрики** — `g_httpDuration->observe()`, при ошибках `g_httpErrors->inc()`
5. **Логирование** — код ответа, URI, клиент, время

### 2.3 Добавление нового endpoint

1. Создать `src/handler/my_handler.h`:
   - Класс, наследующий `HTTPRequestHandler`
   - Реализовать `handleRequest()`
   - Подключить `request_counter.h` для метрик

2. Зарегистрировать в `router_factory.h`:
   - Добавить `#include "my_handler.h"`
   - Добавить условие в `createRequestHandler()`: `if (uri == "/path" && method == "GET") return new MyHandler();`

3. При необходимости обновить Swagger в `swagger_handler.h`

### 2.4 Конфигурация из переменных окружения

Используется `Poco::Environment`:

```cpp
unsigned short port = 8080;
if (Environment::has("PORT")) {
    port = static_cast<unsigned short>(NumberParser::parse(Environment::get("PORT")));
}
std::string level = Environment::get("LOG_LEVEL", "information");  // значение по умолчанию
```

---

## 3. Работа с метриками Prometheus

### 3.1 Компоненты POCO Prometheus

- **Registry** — реестр метрик, по умолчанию `Registry::defaultRegistry()`
- **Counter** — монотонно возрастающий счётчик (`inc()`)
- **Histogram** — распределение значений (бакеты), `observe(value)`
- **MetricsRequestHandler** — готовый handler, экспортирующий метрики в формате Prometheus

### 3.2 Метрики в проекте

| Метрика | Тип | Описание |
|---------|-----|----------|
| `http_requests_total` | Counter | Общее число HTTP-запросов |
| `http_errors_total` | Counter | Число ошибочных ответов (4xx, 5xx) |
| `http_request_duration_seconds` | Histogram | Время обработки запроса в секундах |

### 3.3 Регистрация и использование

Метрики создаются в `main()` и передаются в handlers через глобальные указатели:

```cpp
// main.cpp
static Prometheus::Counter httpRequests("http_requests_total");
httpRequests.help("Total number of HTTP requests");
handlers::g_httpRequests = &httpRequests;

static Prometheus::Histogram httpDuration("http_request_duration_seconds");
httpDuration.help("HTTP request duration in seconds");
httpDuration.buckets({0.0001, 0.0005, 0.001, 0.0025, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0});
handlers::g_httpDuration = &httpDuration;
```

В handler'е:

```cpp
if (g_httpRequests) g_httpRequests->inc();
// ... обработка ...
double seconds = static_cast<double>(duration.totalMicroseconds()) / 1000000.0;
if (g_httpDuration) g_httpDuration->observe(seconds);
if (error) { if (g_httpErrors) g_httpErrors->inc(); }
```

### 3.4 Endpoint /metrics

`MetricsHandler` делегирует вызов `Poco::Prometheus::MetricsRequestHandler`, который экспортирует все метрики из `Registry::defaultRegistry()` в формате [Prometheus text exposition](https://prometheus.io/docs/instrumenting/exposition_formats/).

### 3.5 Важные моменты

- **Точность времени**: используйте `totalMicroseconds()`, а не `totalMilliseconds()`, иначе запросы < 1 мс дают 0
- **Бакеты гистограммы**: для быстрых API добавьте мелкие бакеты (0.0001, 0.0005, 0.001 с)
- **Метрики с labels**: Counter и Histogram поддерживают labels для разбивки по endpoint, коду ответа и т.д.

---

## 4. Работа с JWT и аутентификацией

### 4.1 Схема аутентификации

1. **Получение токена**: `POST /api/v1/auth` с Basic auth → JWT в ответе
2. **Использование токена**: `GET /api/v1/helloworld_jwt` с заголовком `Authorization: Bearer <token>`

### 4.2 Basic auth → JWT (AuthHandler)

- Используется `Poco::Net::HTTPBasicCredentials(request)` для извлечения username/password из заголовка `Authorization: Basic <base64>`
- При отсутствии Basic auth выбрасывается `NotAuthenticatedException`
- JWT создаётся через `Poco::JWT::Token` и `Poco::JWT::Signer`
- Секрет подписи берётся из переменной окружения `JWT_SECRET`

```cpp
Poco::JWT::Token token;
token.setType("JWT");
token.setSubject(username);
token.setIssuedAt(Poco::Timestamp());
token.payload().set("username", username);
Poco::JWT::Signer signer(g_jwtSecret);
std::string jwt = signer.sign(token, Poco::JWT::Signer::ALGO_HS256);
```

### 4.3 Проверка Bearer токена (HelloWorldJwtHandler)

- Извлечение: `request.get("Authorization", "")` → проверка префикса `"Bearer "`
- Верификация: `signer.verify(jwt)` — проверяет подпись и возвращает распарсенный Token
- При невалидном токене — исключение, ответ 401

```cpp
std::string authHeader = request.get("Authorization", "");
if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
    // 401 Bearer token required
}
std::string jwt = authHeader.substr(7);
Poco::JWT::Token token = signer.verify(jwt);
std::string username = token.getSubject();
Poco::Timestamp iat = token.getIssuedAt();
```

### 4.4 Конфигурация

- **JWT_SECRET** — обязательная переменная окружения для подписи и проверки токенов
- Хранится в `handlers::g_jwtSecret`, инициализируется в `main()` из `Environment::get("JWT_SECRET", "")`

### 4.5 TODO: проверка пароля

В `AuthHandler` пароль пока не проверяется. Рекомендуется добавить:

```cpp
// Сравнение с ожидаемым паролем из конфигурации
std::string expectedPassword = Environment::get("AUTH_PASSWORD", "");
if (password != expectedPassword) {
    response.setStatus(HTTPResponse::HTTP_UNAUTHORIZED);
    // ...
}
```

---

## 5. Принцип нагрузочного тестирования

### 5.1 Инструмент wrk

[wrk](https://github.com/wg/wrk) — HTTP-бенчмарк с мультипоточностью и Lua-скриптами.

Основные параметры:
- `-t` — число потоков
- `-c` — число одновременных соединений
- `-d` — длительность теста (например, `30s`, `1m`)

### 5.2 Запуск нагрузочного теста

**Локально** (если установлен wrk):
```bash
cd loadtest
./run.sh
```

**С параметрами**:
```bash
./run.sh http://localhost:8080/api/v1/helloworld 4 100 30s
# URL, threads, connections, duration
```

**Через Docker** (если wrk не установлен):
```bash
./run.sh
# Скрипт автоматически использует docker run satishsverma/wrk
```

**Через docker-compose**:
```bash
docker compose up -d poco_template_server
docker compose run --rm --profile loadtest loadtest
```

### 5.3 Интерпретация результатов wrk

Пример вывода:
```
Running 30s test @ http://localhost:8080/api/v1/helloworld
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.23ms    0.89ms  12.34ms   89.12%
    Req/Sec    20.45k     2.11k   24.00k    76.32%
  2445678 requests in 30.01s, 156.78MB read
Requests/sec:  81512.34
Transfer/sec:      5.22MB
```

- **Latency** — задержка ответа (средняя, макс.)
- **Req/Sec** — запросов в секунду на поток
- **Requests/sec** — общая пропускная способность

### 5.4 Связь с метриками Prometheus

После нагрузочного теста проверьте `/metrics`:

- `http_requests_total` — должен совпадать с числом запросов wrk
- `http_request_duration_seconds` — распределение задержек (бакеты, sum)
- `http_errors_total` — число ошибок (должно быть 0 при успешном тесте)

### 5.5 Рекомендации по нагрузочному тестированию

1. **Прогрев** — сделайте короткий тест перед основным, чтобы стабилизировать кэши и пулы
2. **Постепенное увеличение** — начните с малого числа соединений, увеличивайте до появления деградации
3. **Мониторинг** — во время теста смотрите метрики и логи сервера
4. **Реалистичные сценарии** — тестируйте разные endpoint'ы, включая защищённые (JWT)

---

## 6. Устройства логгирования (Logging)

### 6.1 Архитектура логгирования POCO

Система логгирования POCO состоит из трёх компонентов:

| Компонент | Назначение |
|-----------|------------|
| **Logger** | Принимает сообщения, фильтрует по уровню, передаёт в Channel |
| **Channel** | Устройство вывода — куда попадает сообщение (консоль, файл, syslog) |
| **Formatter** | Форматирует сообщение перед выводом (шаблон, время, уровень) |

Цепочка: `Logger` → `FormattingChannel` (Formatter + Channel) → вывод.

### 6.2 Устройства вывода (Channels)

- **ConsoleChannel** — вывод в `std::clog` (stderr). Используется по умолчанию, если не настроено иное.
- **FileChannel** — запись в файл с поддержкой ротации по размеру или времени, сжатия (gzip), очистки старых файлов.
- **SyslogChannel** — отправка в syslog (Unix).
- **SimpleFileChannel** — простая запись в файл без ротации.
- **SplitterChannel** — рассылка одного сообщения в несколько каналов (например, консоль + файл).

### 6.3 Уровни логирования

От самого детального к самому строгому:

| Уровень | Константа | Использование |
|---------|-----------|---------------|
| trace | PRIO_TRACE | Подробная отладка |
| debug | PRIO_DEBUG | Отладочная информация |
| information | PRIO_INFORMATION | Обычный ход работы |
| notice | PRIO_NOTICE | Важные события |
| warning | PRIO_WARNING | Предупреждения (4xx) |
| error | PRIO_ERROR | Ошибки (5xx, исключения) |
| critical | PRIO_CRITICAL | Критические сбои |
| fatal | PRIO_FATAL | Фатальные ошибки |
| none | — | Отключить логирование |

Сообщение выводится, если его уровень **не ниже** установленного для логгера.

### 6.4 Конфигурация в проекте

Уровень задаётся переменной окружения **LOG_LEVEL** и применяется к корневому логгеру:

```cpp
// main.cpp, configureLogging()
std::string level = Environment::get("LOG_LEVEL", "information");
Logger::root().setLevel(prio);
```

Поддерживаемые значения: `trace`, `debug`, `information`, `info`, `notice`, `warning`, `warn`, `error`, `critical`, `fatal`, `none`.

По умолчанию используется **ConsoleChannel** — логи идут в stderr контейнера/процесса.

### 6.5 Именованные логгеры

В проекте используется логгер `"Server"`:

```cpp
auto& logger = Poco::Logger::get("Server");
logger.information("200 GET /api/v1/helloworld from %s, %.2f ms", client, ms);
logger.warning("401 POST /api/v1/auth from %s - Basic authentication required", client);
logger.error("500 POST /api/v1/auth from %s - %s", client, e.what());
```

Именованные логгеры наследуют уровень и канал от корневого. Можно задать отдельные уровни для разных имён (например, `Server` — information, `Auth` — debug).

### 6.6 Расширенная настройка (LoggingConfigurator)

Для вывода в файл, syslog или настройки формата используется `LoggingConfigurator` и конфигурация (файл или свойства):

```cpp
// Пример: вывод в файл с ротацией
// Требует Poco::Util::LoggingConfigurator и конфигурацию
```

Типичная конфигурация в properties-формате:

```properties
logging.loggers.root.level = information
logging.loggers.root.channel = c1
logging.channels.c1.class = FileChannel
logging.channels.c1.path = /var/log/app.log
logging.channels.c1.rotation = 10 M
logging.channels.c1.archive = number
```

### 6.7 Формат сообщений в проекте

Текущий формат (задаётся по умолчанию POCO):

```
<timestamp> <level> <logger>: <message>
```

Пример:
```
2025-02-21 12:34:56.123 Information Server: 200 GET /api/v1/helloworld from 127.0.0.1:54321, 0.52 ms
```

Для кастомного формата используется `PatternFormatter` с шаблоном, например: `%Y-%m-%d %H:%M:%S [%p] %t`.

---

## Приложение: быстрый старт

```bash
# Сборка и запуск
docker compose up --build

# Получить JWT
curl -X POST -u "user:password" http://localhost:8080/api/v1/auth

# Вызвать защищённый endpoint
TOKEN=$(curl -s -X POST -u "user:password" http://localhost:8080/api/v1/auth | jq -r .token)
curl -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/v1/helloworld_jwt

# Метрики
curl http://localhost:8080/metrics

# Нагрузочный тест
cd loadtest && ./run.sh
```
