# Poco Template Server (PostgreSQL)

REST сервер на C++ с использованием POCO и PostgreSQL.

## Что реализовано

- CRUD для `Organization` (`id`, `name`, `address`)
- CRUD для `User` (`id`, `first_name`, `last_name`, `title`, `email`, `login`, `password`, `organization_id nullable`)
- Хранение данных в PostgreSQL через `Poco::Data::PostgreSQL`
- При старте сервиса создаются таблицы и индексы, если их нет
- Swagger доступен по `GET /swagger.yaml`

## Endpoints

- `GET|POST /api/v1/organizations`
- `GET|PUT|DELETE /api/v1/organizations/{id}`
- `GET|POST /api/v1/users`
- `GET /api/v1/users` поддерживает поиск по подстрокам через query-параметры: `first_name`, `last_name`, `organization`
- `GET|PUT|DELETE /api/v1/users/{id}`
- `GET /metrics`
- `GET /swagger.yaml`

## Переменные окружения

- `PORT` (по умолчанию `8080`)
- `LOG_LEVEL` (`trace|debug|information|notice|warning|error|critical|fatal|none`)
- `PGHOST` (по умолчанию `localhost`)
- `PGPORT` (по умолчанию `5432`)
- `PGDATABASE` (по умолчанию `poco_template`)
- `PGUSER` (по умолчанию `postgres`)
- `PGPASSWORD` (по умолчанию `postgres`)

## Сборка и запуск

```bash
mkdir build && cd build
cmake ..
cmake --build .
./poco_template_server
```

## Docker Compose

```bash
docker compose up --build
```

Сервис поднимет приложение и PostgreSQL. Таблицы и индексы создаются автоматически при старте.
