curl -X POST http://localhost:8080/query \
-H "Content-Type: text/plain" \
-d 'query {
    all_users{
        id,
        first_name,
        last_name,
        email,
        title,
        login,
        password
    }
}'
