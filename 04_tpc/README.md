# TPC — Two-Phase Commit (двухфазная фиксация)

Упрощённая демонстрация паттерна двухфазной фиксации транзакций для курса «Архитектура программных систем».

## Описание

Пример имитирует двухфазный протокол коммита транзакций в памяти:

1. **Фаза подготовки** — создание транзакции (status=0)
2. **Фаза фиксации** — commit (status=1) или abort (status=2)

## Endpoints

- `POST /users` — создание пользователя (создаёт транзакцию)
- `POST /tx/{id}/1` — commit транзакции
- `POST /tx/{id}/2` — abort транзакции
- `GET /users` — список пользователей (применяет закоммиченные транзакции)
- `GET /users/{user_id}` — пользователь по ID

## Модель Transaction

- `status=0` — created (транзакция создана)
- `status=1` — committed (применена)
- `status=2` — aborted (отменена)

## Запуск

```bash
uvicorn main:app --host 0.0.0.0 --port 8000
```

## Пример сценария

```bash
# Создать пользователя (получаем Transaction)
curl -X POST "http://localhost:8000/users" -H "Content-Type: application/json" \
  -d '{"id": 1, "name": "John", "email": "john@example.com"}'

# Закоммитить транзакцию
curl -X POST "http://localhost:8000/tx/1/1"

# Или отменить
curl -X POST "http://localhost:8000/tx/1/2"
```
