curl -X POST http://localhost:8080/query \
-H "Content-Type: text/plain" \
-d 'mutation {
    add_user(
        first_name: "Konstantin",
        last_name: "Konstantinoff",
        email: "ppkk@yandex.ru",
        title: "mr",
        login: "ppkktof",
        password: "12345678"
    )
}'
