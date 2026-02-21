# Circuit Breaker — размыкатель цепи

Реализация паттерна Circuit Breaker для устойчивости к сбоям внешних сервисов. Курс «Архитектура программных систем».

## Описание

Сервис заказов (Order Service) обращается к сервису пользователей (User Service). При многократных сбоях User Service Circuit Breaker переходит в состояние **OPEN** и блокирует запросы, предотвращая каскадные отказы.

## Состояния Circuit Breaker

| Состояние | Описание |
|-----------|----------|
| **CLOSE** | Нормальная работа, запросы проходят |
| **OPEN** | Слишком много ошибок — запросы блокируются |
| **SEMI_OPEN** | Пробный период — ограниченное число запросов для проверки восстановления |

## Параметры (circuit_breaker.py)

- `FAIL_COUNT = 5` — порог ошибок для перехода в OPEN
- `TIME_LIMIT = 5` сек — время ожидания перед переходом в SEMI_OPEN
- `SUCCESS_LIMIT = 5` — успешных запросов в SEMI_OPEN для возврата в CLOSE

## Компоненты

- **user_service.py** — сервис пользователей (порт 8001)
- **order_service.py** — сервис заказов, вызывает user_service через Circuit Breaker (порт 8002)
- **circuit_breaker.py** — реализация паттерна

## Запуск

```bash
# Терминал 1 — User Service
uvicorn user_service:app --host 0.0.0.0 --port 8001

# Терминал 2 — Order Service
uvicorn order_service:app --host 0.0.0.0 --port 8002
```

Остановите User Service, чтобы увидеть срабатывание Circuit Breaker при запросах к Order Service.
