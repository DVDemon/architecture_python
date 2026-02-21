# MongoDB — документоориентированная БД

Примеры работы с MongoDB для курса «Архитектура программных систем».

## Описание

REST API для управления пользователями с хранением в MongoDB. Демонстрирует:

- Подключение к MongoDB через PyMongo
- Создание индексов
- CRUD-операции с коллекциями

## Структура

- **main.py** — FastAPI приложение с CRUD для пользователей
- **01_example.py** — базовый пример
- **cluster/** — конфигурация replica set (mongo1, mongo2, mongo3)
- **commands.sh** — команды для MongoDB shell

## Endpoints

- `POST /users/` — создание пользователя
- `GET /users/{user_id}` — получение по ID
- `GET /users/` — список всех пользователей

## Запуск

Требуется MongoDB. По умолчанию подключается к `mongodb:27017`.

```bash
# Запуск replica set (опционально)
cd cluster && docker-compose up -d
# Настройка: ./mongo_setup.sh

# Запуск приложения
uvicorn main:app --host 0.0.0.0 --port 8000
```

## Replica Set

В `cluster/docker-compose.yml` — конфигурация из трёх узлов MongoDB для отказоустойчивости.
