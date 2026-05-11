# Домашнее задание 05. Кеширование и rate limiting

В рамках работы доработана система управления библиотекой на C++ с использованием Poco, PostgreSQL и Redis.

---
## Вариант 15 - Система управления библиотекой

Приложение должно содержать следующие данные:

- Пользователь  
- Книга  
- Выдача  

Необходимо реализовать API:

- Создание нового пользователя  
- Поиск пользователя по логину  
- Поиск пользователя по маске имя и фамилии  
- Добавление книги в библиотеку  
- Поиск книги по названию  
- Поиск книги по автору  
- Создание выдачи книги пользователю  
- Получение списка выданных книг пользователя  
- Возврат книги  

---

## Что добавлено в этой работе?

В проект добавлены два механизма оптимизации:

- Redis-кеширование для часто вызываемых GET-запросов;
- rate limiting для endpoint авторизации.

Подробное описание выбора стратегии, TTL, инвалидации кеша и лимитов приведено в файле `performance_design.md`.

## Используемые сервисы

Проект запускается через Docker Compose и включает:

- `library_api` - REST API на C++ и Poco;
- `library_postgres` - PostgreSQL для хранения данных;
- `library_redis` - Redis для кеша и счётчиков rate limiting.

После запуска доступны:

- API: `http://localhost:8080`;
- PostgreSQL: порт `5432`;
- Redis: порт `6379`.

## Кешируемые endpoints

Кеширование добавлено для двух endpoints:

```http
GET /api/v1/books/search
GET /api/v1/users/{userId}/loans
```

В ответ добавляется заголовок `X-Cache`.

Пример проверки:

```bash
curl -i "http://localhost:8080/api/v1/books/search?title=War"
curl -i "http://localhost:8080/api/v1/books/search?title=War"
```

Ожидаемый результат:

```text
первый запрос  - X-Cache: MISS
повторный      - X-Cache: HIT
```

## Rate limiting

Rate limiting добавлен для авторизации:

```http
POST /api/v1/auth/login
```

При превышении лимита сервер возвращает:

```http
429 Too Many Requests
```

В ответе также передаются заголовки:

- `X-RateLimit-Limit`;
- `X-RateLimit-Remaining`;
- `X-RateLimit-Reset`.

## Примеры запросов

Получение списка выдач пользователя:

```bash
curl -i "http://localhost:8080/api/v1/users/1/loans"
```

Создание файла для проверки авторизации:

```bash
echo '{"login":"reader01","password":"secret123"}' > login.json
```

Для PowerShell:

```powershell
Set-Content -Path login.json -Value '{"login":"reader01","password":"secret123"}' -Encoding ascii
```

Запрос авторизации:

```bash
curl -i -X POST "http://localhost:8080/api/v1/auth/login" \
  -H "Content-Type: application/json" \
  --data-binary "@login.json"
```

## Запуск проекта

### Сборка и запуск

```bash
docker compose up --build
```

### Остановка и удаление контейнеров

```bash
docker compose down -v
```