wrk.method = "POST"
wrk.headers["Content-Type"] = "text/plain"
wrk.body = 'query { all_users { id, first_name, last_name, email, title, login, password } }'