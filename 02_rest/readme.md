# REST API — примеры на FastAPI

Примеры построения REST API на Python/FastAPI для курса «Архитектура программных систем».

## Описание

Пошаговое усложнение REST API: от простого «Hello World» до CRUD с JWT-аутентификацией.

## Примеры

| Файл | Описание |
|------|----------|
| **01_simple.py** | Минимальный API: `GET /` возвращает `{"Hello": "World"}` |
| **02_user.py** | CRUD для пользователей (память): GET, POST, PUT, DELETE `/users` |
| **03_jwt.py** | CRUD с JWT-аутентификацией: OAuth2, Bearer токены |

## Запуск

```bash
# Установка зависимостей
pip install fastapi uvicorn python-jose passlib bcrypt

# Запуск (например, 02_user.py)
uvicorn 02_user:app --host 0.0.0.0 --port 8000
```

## Документация

- Swagger UI: `http://localhost:8000/docs`
- OpenAPI JSON: `http://localhost:8000/openapi.json`

## Пример запроса с JWT (03_jwt.py)

```bash
# Получение токена
curl -X POST "http://localhost:8000/token" -d "username=admin&password=secret"

# Запрос с токеном
curl -X GET "http://localhost:8000/users" -H "Authorization: Bearer <token>"
```
