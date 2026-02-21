# Structurizr — документация архитектуры

Пример ведения архитектурной документации с помощью [Structurizr DSL](https://structurizr.com/help/dsl) для курса «Архитектура программных систем».

## Описание

Structurizr — инструмент для создания и поддержки архитектурной документации «как код». Демонстрационный проект описывает систему «Мобильный телохранитель» — сервис наблюдения за детьми с использованием дронов.

## Структура

- **workspace.dsl** — главный файл с моделью и представлениями
- **model.dsl** — описание компонентов системы (CRM, Billing, Inventory, Tracking, BPM)
- **deployment_model.dsl** — модель развёртывания
- **documentation/** — текстовые документы по C4-методологии:
  - Введение и цели
  - Ограничения
  - Контекст и область
  - Стратегия решения
  - Блоки и компоненты
  - Runtime view
  - Deployment view
  - Скрещивающиеся концепции
  - Качество
  - Риски и технический долг
  - Глоссарий
- **decisions/** — ADR (Architecture Decision Records)
- **from_lecture/** — материалы с лекций


## Запуск

Используйте [Structurizr Lite](https://structurizr.com/help/lite) или CLI для визуализации:

```bash
# Установка Structurizr CLI
# https://structurizr.com/help/cli

structurizr export -workspace workspace.dsl -format plantuml
```
