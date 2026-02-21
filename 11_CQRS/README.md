# CQRS — Command Query Responsibility Segregation

Пример паттерна CQRS с разделением команд и запросов через Kafka. Курс «Архитектура программных систем».

## Описание

CQRS разделяет операции записи (Commands) и чтения (Queries):

- **main.py** — Command side: приём команд создания пользователей, публикация в Kafka
- **writer.py** — Consumer: читает из Kafka и записывает в PostgreSQL

Запросы на чтение (`GET /users`, `GET /users/{id}`) идут в PostgreSQL; команды на создание (`POST /users`) — в Kafka, откуда writer асинхронно пишет в БД.

## Архитектура

```
Client → POST /users → main.py → Kafka (user_topic)
                              → writer.py → PostgreSQL

Client → GET /users  → main.py → PostgreSQL
```

## Запуск

Требуются PostgreSQL и Kafka.

```bash
# Терминал 1 — Writer (Consumer, запись в БД)
python writer.py

# Терминал 2 — API (Command + Query)
uvicorn main:app --host 0.0.0.0 --port 8000
```

## Особенности

- Запись асинхронная: после `POST /users` данные появятся в БД после обработки writer
- Чтение синхронное: `GET /users` возвращает данные из PostgreSQL
