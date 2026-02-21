# Poco Template Server

REST сервер на C++ с использованием библиотеки POCO.

## Endpoints

- `GET /api/v1/helloworld` — возвращает приветствие в JSON
- `GET /api/v1/helloworld_jwt` — приветствие с данными из JWT (требует Bearer токен)
- `POST /api/v1/auth` — Basic auth → JWT токен
- `GET /metrics` — метрики Prometheus
- `GET /swagger.yaml` — OpenAPI спецификация в формате YAML

## Переменные окружения

- `PORT` — порт сервера (по умолчанию: 8080)
- `LOG_LEVEL` — уровень логирования: trace, debug, information, notice, warning, error, critical, fatal, none
- `JWT_SECRET` — секрет для подписи JWT токенов (обязательно для auth и helloworld_jwt)

## Сборка

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Запуск

```bash
./build/poco_template_server
```

## Docker

```bash
docker build -t poco_template_server .
docker run -p 8080:8080 -e LOG_LEVEL=debug poco_template_server
```
