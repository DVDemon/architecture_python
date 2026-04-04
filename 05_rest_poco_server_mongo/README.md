# Poco Template Server (MongoDB)

REST сервер на C++ с использованием POCO и MongoDB.

## Что реализовано

- CRUD для `Organization` (`id`, `name`, `address`)
- CRUD для `User` (`id`, `first_name`, `last_name`, `title`, `email`, `login`, `password`, `organization_id` nullable)
- Хранение данных в MongoDB через `Poco::MongoDB` (wire protocol OP_MSG)
- Числовые `id` в API: счётчики в коллекции `counters` (`findAndModify` + `$inc`)
- При старте создаются уникальные индексы по полям `id`, `email`, `login` (для `users`) и `id` (для `organizations`)
- При удалении организации у связанных пользователей сбрасывается `organization_id` (аналог `ON DELETE SET NULL`)
- Swagger доступен по `GET /swagger.yaml`

## Endpoints

- `GET|POST /api/v1/organizations`
- `GET|PUT|DELETE /api/v1/organizations/{id}`
- `GET|POST /api/v1/users`
- `GET /api/v1/users` поддерживает фильтрацию через query-параметры `first_name`, `last_name`, `organization` (регистронезависимое вхождение подстроки через BSON regex)
- `GET|PUT|DELETE /api/v1/users/{id}`
- `GET /metrics`
- `GET /swagger.yaml`

## Примеры запросов (curl)

Ниже `BASE` — URL сервера (по умолчанию порт `8080`). Подставьте реальные `id` из ответов `POST` / `GET`.

```bash
BASE=http://localhost:8080
```

Подключение к тем же данным в оболочке (имя БД = `MONGO_DATABASE`, по умолчанию `poco_template`):

```bash
mongosh "mongodb://localhost:27017/poco_template"
# из DevContainer к сервису compose: mongosh "mongodb://mongodb:27017/poco_template"
```

### Organizations

**HTTP**

```bash
# Список организаций
curl -sS "$BASE/api/v1/organizations"

# Создать организацию
curl -sS -X POST "$BASE/api/v1/organizations" \
  -H 'Content-Type: application/json' \
  -d '{"name":"Acme Corp","address":"1 Infinite Loop"}'

# Получить по id (замените 1 на свой id)
curl -sS "$BASE/api/v1/organizations/1"

# Обновить
curl -sS -X PUT "$BASE/api/v1/organizations/1" \
  -H 'Content-Type: application/json' \
  -d '{"name":"Acme Ltd","address":"2 Market St"}'

# Удалить
curl -sS -X DELETE "$BASE/api/v1/organizations/1"
```

**MongoDB (mongosh)** — коллекция `organizations`, бизнес-ключ числовой `id`:

```javascript
// GET /api/v1/organizations — все, сортировка по id (как в сервере)
db.organizations.find().sort({ id: 1 })

// POST … — id выдаёт приложение (counters); руками только для отладки:
// db.organizations.insertOne({ id: NumberLong(1), name: "Acme Corp", address: "1 Infinite Loop" })

// GET /api/v1/organizations/1
db.organizations.findOne({ id: 1 })

// PUT …/1
db.organizations.updateOne(
  { id: 1 },
  { $set: { name: "Acme Ltd", address: "2 Market St" } }
)

// DELETE …/1 (сервер перед этим сбрасывает organization_id у пользователей)
db.users.updateMany({ organization_id: 1 }, { $set: { organization_id: null } })
db.organizations.deleteOne({ id: 1 })
```

### Users

**HTTP**

```bash
# Список пользователей
curl -sS "$BASE/api/v1/users"

# Поиск по подстрокам (query-параметры можно комбинировать)
curl -sS "$BASE/api/v1/users?first_name=john&last_name=doe"
curl -sS "$BASE/api/v1/users?organization=Acme"

# Создать пользователя (organization_id — id существующей организации или опустите поле)
curl -sS -X POST "$BASE/api/v1/users" \
  -H 'Content-Type: application/json' \
  -d '{
    "first_name":"Jane",
    "last_name":"Doe",
    "title":"Engineer",
    "email":"jane@example.com",
    "login":"janedoe",
    "password":"secret",
    "organization_id": 1
  }'

# Получить по id
curl -sS "$BASE/api/v1/users/1"

# Обновить
curl -sS -X PUT "$BASE/api/v1/users/1" \
  -H 'Content-Type: application/json' \
  -d '{
    "first_name":"Jane",
    "last_name":"Doe",
    "title":"Senior Engineer",
    "email":"jane@example.com",
    "login":"janedoe",
    "password":"newsecret",
    "organization_id": 1
  }'

# Удалить (ответ 204 без тела)
curl -sS -i -X DELETE "$BASE/api/v1/users/1"
```

**MongoDB (mongosh)** — коллекция `users`; поиск в API реализован через **регистронезависимый regex** (`$options: "i"`):

```javascript
// GET /api/v1/users
db.users.find().sort({ id: 1 })

// GET …?first_name=john&last_name=doe
db.users.find({
  $and: [
    { first_name: { $regex: "john", $options: "i" } },
    { last_name: { $regex: "doe", $options: "i" } },
  ],
}).sort({ id: 1 })

// GET …?organization=Acme — сначала организации по имени, затем пользователи по organization_id
const orgIds = db.organizations
  .find({ name: { $regex: "Acme", $options: "i" } }, { id: 1 })
  .toArray()
  .map((o) => o.id)
db.users.find({ organization_id: { $in: orgIds } }).sort({ id: 1 })

// POST … — id выдаёт приложение; для проверки схемы документа:
// db.users.insertOne({ id: NumberLong(1), first_name: "Jane", last_name: "Doe", title: "Engineer",
//   email: "jane@example.com", login: "janedoe", password: "secret", organization_id: 1 })

// GET /api/v1/users/1
db.users.findOne({ id: 1 })

// PUT …/1
db.users.updateOne(
  { id: 1 },
  {
    $set: {
      first_name: "Jane",
      last_name: "Doe",
      title: "Senior Engineer",
      email: "jane@example.com",
      login: "janedoe",
      password: "newsecret",
      organization_id: 1,
    },
  }
)

// DELETE …/1
db.users.deleteOne({ id: 1 })
```

Счётчики для новых `id` (как в приложении):

```javascript
db.counters.find()
// пользователи: _id "users"; организации: _id "organizations"
```

### Служебные

```bash
curl -sS "$BASE/swagger.yaml" | head
curl -sS "$BASE/metrics" | head
```

К OpenAPI и Prometheus в MongoDB аналогов нет.

## Переменные окружения

- `PORT` (по умолчанию `8080`)
- `LOG_LEVEL` (`trace|debug|information|notice|warning|error|critical|fatal|none`)
- `MONGO_HOST` (по умолчанию `localhost`; в DevContainer сервис называется `mongodb`)
- `MONGO_PORT` (по умолчанию `27017`)
- `MONGO_DATABASE` (по умолчанию `poco_template`)

## Сборка и запуск

Требуется POCO с компонентом **MongoDB** (в образе DevContainer POCO 1.15 уже собран из исходников).

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
cmake --build .
MONGO_HOST=mongodb MONGO_PORT=27017 ./poco_template_server
```

Либо из корня модуля:

```bash
./run_service.sh up
```

(по умолчанию `MONGO_HOST=mongodb` — как в Docker Compose этого проекта.)

## Docker Compose

```bash
docker compose up --build
```

Поднимаются приложение и MongoDB (`mongo:5.0`). Индексы создаются при старте приложения.

## DevContainer

В корневом `.devcontainer/docker-compose.yml` уже есть сервис `mongo:5.0` с именем хоста `mongodb` и портом `27017`. Для запуска сервера из контейнера задайте `MONGO_HOST=mongodb`.
