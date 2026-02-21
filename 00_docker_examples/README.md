# Docker — примеры контейнеризации

Примеры использования Docker для курса «Архитектура программных систем».

## Примеры

| Пример | Описание |
|--------|----------|
| **Example01** | Базовый Dockerfile: образ на Ubuntu с nginx и статическим HTML |
| **Example01_Builder** | Multi-stage build для Next.js приложения |
| **Example02** | docker-compose с Apache HTTP Server и volume для статики |
| **Example03** | docker-compose с nginx и volume для статики |
| **Example04** | Полный стек: MySQL/MariaDB + приложение sql_test с healthcheck и сетью |

## Example01 — простой образ

Создание образа с nginx и статической страницей:

```bash
docker build . -t my_nginx
docker run --rm -p 80:80 my_nginx
```

## Example01_Builder — multi-stage build

Демонстрация multi-stage сборки для уменьшения размера итогового образа (на примере Next.js).

## Example02 — Apache + volume

Запуск Apache с монтированием текущей директории в `/usr/local/apache2/htdocs`:

```bash
docker-compose up -d
# Приложение на http://localhost:8080
```

## Example03 — nginx + volume

Аналогично Example02, но с nginx:

```bash
docker-compose up -d
# Приложение на http://localhost:8080
```

## Example04 — MySQL + приложение

Полноценный стек с базой данных:

- **db-node-1** — MariaDB/MySQL с healthcheck
- **sql_test** — приложение для тестирования подключения к БД
- Сеть `vpcbr` с фиксированными IP

```bash
docker-compose up -d
```
