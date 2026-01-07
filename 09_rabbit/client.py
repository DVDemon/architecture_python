import pika
import json

# Данные о пользователе


# Подключение к RabbitMQ
connection = pika.BlockingConnection(pika.ConnectionParameters('rabbitmq'))
channel = connection.channel()

# Создание очереди (если она не существует)
channel.queue_declare(queue='user_queue')

# Преобразование данных в JSON и отправка в очередь
for i in range(1000000):
    user_data = {
        "id": i,
        "name": "John Doe",
        "email": "john.doe@example.com"
    }
    channel.basic_publish(exchange='',
                        routing_key='user_queue',
                        body=json.dumps(user_data))

print(f" [x] Sent user data: {user_data}")

# Закрытие соединения
connection.close()