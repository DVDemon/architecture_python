# RabbitMQ — очереди сообщений

Примеры работы с RabbitMQ для асинхронной передачи сообщений. Курс «Архитектура программных систем».

## Описание

Демонстрация паттерна «Producer — Consumer» через очередь RabbitMQ:

- **client.py** — Producer: отправляет сообщения с данными пользователей в очередь `user_queue`
- **server.py** — Consumer: получает и обрабатывает сообщения из очереди

## Запуск

Требуется RabbitMQ (например, `docker run -d -p 5672:5672 rabbitmq`).

```bash
# Терминал 1 — Consumer (сначала запустить)
python server.py

# Терминал 2 — Producer
python client.py
```

## Зависимости

```bash
pip install pika
```

## Формат сообщений

JSON с полями: `id`, `name`, `email`.
