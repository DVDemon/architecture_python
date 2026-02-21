# Redis — кэширование

Пример использования Redis в качестве кэша для REST API с PostgreSQL для курса «Архитектура программных систем».

## Описание

FastAPI приложение с двухуровневым хранением:

- **PostgreSQL** — основное хранилище (SQLAlchemy)
- **Redis** — кэш для чтения пользователей по ID (TTL 180 сек)

При запросе `GET /users/{id}` сначала проверяется кэш; при промахе данные читаются из БД и сохраняются в Redis.

## Endpoints

- `POST /users/` — создание пользователя (только БД)
- `GET /users/{user_id}` — получение по ID (кэш + БД)
- `GET /users/` — список всех пользователей

## Запуск

Требуются PostgreSQL и Redis.

```bash
export REDIS_URL="redis://localhost:6379/0"
export DATABASE_URL="postgresql://stud:stud@localhost/archdb"
uvicorn main:app --host 0.0.0.0 --port 8000
```

## Дополнительно

- **filldb.py** — заполнение БД тестовыми данными
- **get.lua** / **get_no_cache.lua** — Lua-скрипты для Redis (опционально)
- **test.sh** / **test_no_cache.sh** — скрипты нагрузочного тестирования
