# Poco Template Server (MongoDB + Redis)

REST сервер на C++ с использованием POCO, MongoDB и **кэшем пользователей в Redis** (`Poco::Redis`).

## Что реализовано

- CRUD для `Organization` (`id`, `name`, `address`)
- CRUD для `User` (`id`, `first_name`, `last_name`, `title`, `email`, `login`, `password`, `organization_id` nullable)
- Хранение данных в MongoDB через `Poco::MongoDB` (wire protocol OP_MSG)
- **Кэш по `user_id` в Redis**: ключ `user:{id}`, значение — JSON пользователя (как в API); `GET /api/v1/users` и поиск по query-параметрам по-прежнему читают только MongoDB
- **Read-through** для `GET /api/v1/users/{id}`: сначала Redis, при промахе — MongoDB, затем запись в кэш
- **Write-through** для `POST/PUT /api/v1/users` и `DELETE /api/v1/users/{id}`: после успешной записи в MongoDB кэш обновляется (`SET` с TTL) или инвалидируется (`DEL`)
- TTL записей в Redis: **60 секунд** (`SET` с опцией `PX` через `Poco::Redis::Command::set` и `Poco::Timespan`)
- Доступ к Redis из всех HTTP-воркеров сериализуется **`std::mutex`** вокруг одного `Poco::Redis::Client` (потокобезопасность при многопоточной обработке запросов POCO)
- Числовые `id` в API: счётчики в коллекции `counters` (`findAndModify` + `$inc`)
- При старте создаются уникальные индексы по полям `id`, `email`, `login` (для `users`) и `id` (для `organizations`)
- При удалении организации у связанных пользователей сбрасывается `organization_id` (аналог `ON DELETE SET NULL`)
- Swagger доступен по `GET /swagger.yaml`

## Endpoints

- `GET|POST /api/v1/organizations`
- `GET|PUT|DELETE /api/v1/organizations/{id}`
- `GET|POST /api/v1/users`
- `GET /api/v1/users` поддерживает фильтрацию через query-параметры `first_name`, `last_name`, `organization` (регистронезависимое вхождение подстроки через BSON regex)
- `GET|PUT|DELETE /api/v1/users/{id}` — для `{id}` используется кэш Redis (см. выше)
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

Проверка кэша пользователя с `id=1` в Redis:

```bash
redis-cli -h localhost GET user:1
redis-cli -h localhost TTL user:1
# из DevContainer (имя сервиса в корневом compose — cache):
redis-cli -h cache GET user:1
redis-cli -h cache TTL user:1
```

После `TTL` должно оставаться не больше 60 секунд до истечения записи.

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

# Получить по id (read-through: Redis → MongoDB при промахе)
curl -sS "$BASE/api/v1/users/1"

# Обновить (write-through: MongoDB, затем обновление кэша)
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

# Удалить (ответ 204 без тела; ключ user:{id} удаляется из Redis)
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

К OpenAPI и Prometheus в MongoDB/Redis аналогов нет.

## Переменные окружения

- `PORT` (по умолчанию `8080`)
- `LOG_LEVEL` (`trace|debug|information|notice|warning|error|critical|fatal|none`)
- `MONGO_HOST` (по умолчанию `localhost`; в DevContainer сервис называется `mongodb`)
- `MONGO_PORT` (по умолчанию `27017`)
- `MONGO_DATABASE` (по умолчанию `poco_template`)
- `REDIS_HOST` (по умолчанию `localhost`; в корневом `.devcontainer/docker-compose.yml` сервис Redis — **`cache`**)
- `REDIS_PORT` (по умолчанию `6379`)

Если Redis недоступен при первом обращении к кэшу, приложение логирует предупреждение и **продолжает работу только с MongoDB** (кэш для процесса отключается).

## Параметры командной строки

Через `Poco::Util::ServerApplication` (в т.ч. стандартные опции вроде `--daemon` на Unix):

- **`--user-cache=VALUE`** — использовать Redis для кэша пользователя по `id` или нет. Значение **без учёта регистра**: `on` / `off`, `1` / `0`, `true` / `false`, `yes` / `no`. По умолчанию кэш **включён** (`on`). При **`off`** не выполняются ни read-through, ни write-through в Redis (остаётся только MongoDB).

Пример:

```bash
./poco_template_server --user-cache=off
```

## Сборка и запуск

Требуется POCO с компонентами **MongoDB** и **Redis** (в образе DevContainer POCO 1.15 уже собран из исходников).

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
cmake --build .
MONGO_HOST=mongodb MONGO_PORT=27017 REDIS_HOST=cache REDIS_PORT=6379 ./poco_template_server
```

Либо из корня модуля:

```bash
./run_service.sh up
./run_service.sh up --user-cache=off
```

(по умолчанию `MONGO_HOST=mongodb`, `REDIS_HOST=cache` — как в Docker Compose DevContainer / примера ниже. Всё, что указано после подкоманды `up`, передаётся бинарнику. Перед запуском скрипт каждый раз выполняет **инкрементальную** `cmake`/`cmake --build`, чтобы не оставался устаревший `poco_template_server` без новых опций.)

## Docker Compose

```bash
docker compose up --build
```

Поднимаются приложение, MongoDB (`mongo:5.0`) и Redis (`redis:6.2-alpine`, сервис `cache`). Индексы MongoDB создаются при старте приложения.

## DevContainer

В корневом `.devcontainer/docker-compose.yml` уже есть `mongodb` и **`cache`** (Redis на порту `6379`). Для запуска сервера из контейнера задайте `MONGO_HOST=mongodb` и **`REDIS_HOST=cache`** (скрипт `run_service.sh` выставляет это по умолчанию).
