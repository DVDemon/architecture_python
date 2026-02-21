# SQL — реляционные базы данных

Примеры работы с PostgreSQL через SQLAlchemy и ORM для курса «Архитектура программных систем».

## Описание

REST API для управления пользователями с хранением в PostgreSQL. Демонстрирует:

- ORM SQLAlchemy
- Миграции и создание таблиц
- CRUD-операции
- Партиционирование таблиц

## Структура

- **main.py** — FastAPI приложение с CRUD для пользователей (User)
- **commands.sql** — SQL-команды
- **partitioning.sql** — пример партиционирования по диапазону (RANGE)
- **load_json.py** — загрузка JSON в БД

## Endpoints

- `POST /users/` — создание пользователя
- `GET /users/{user_id}` — получение по ID
- `GET /users/` — список всех пользователей

## Запуск

Требуется PostgreSQL. Переменная окружения:

```bash
export DATABASE_URL="postgresql://postgres:postgres@localhost/postgres"
uvicorn main:app --host 0.0.0.0 --port 8000
```

## Партиционирование

В `partitioning.sql` — пример таблицы `orders` с партициями по году (`orders_2019`, `orders_2020`, `orders_2021`).
