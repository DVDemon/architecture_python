import argparse
import json
import os
import random
import sys
import uuid
from urllib.parse import urljoin
from urllib.request import Request, urlopen


def http_json(base_url: str, method: str, path: str, body=None):
    url = urljoin(base_url.rstrip("/") + "/", path.lstrip("/"))
    data = None
    headers = {"Accept": "application/json"}
    if body is not None:
        data = json.dumps(body).encode("utf-8")
        headers["Content-Type"] = "application/json"

    req = Request(url=url, method=method, headers=headers, data=data)
    try:
        with urlopen(req, timeout=30) as resp:
            status = resp.status
            raw = resp.read().decode("utf-8") if resp is not None else ""
            if not raw:
                return status, None
            return status, json.loads(raw)
    except Exception as e:
        # urllib.error.HTTPError is a subclass of Exception;
        # try to include response body for easier debugging.
        status = getattr(e, "code", None)
        try:
            body_text = e.read().decode("utf-8", errors="replace")
        except Exception:
            body_text = ""
        suffix = f", body={body_text}" if body_text else ""
        raise RuntimeError(f"HTTP {method} {url} failed: status={status}{suffix} ({e})") from e


def ensure_organizations(base_url: str, target_orgs: int):
    status, existing = http_json(base_url, "GET", "/api/v1/organizations")
    if status != 200:
        raise RuntimeError(f"Failed to list organizations, status={status}")

    existing = existing or []
    by_name = {o["name"]: o for o in existing if "name" in o}

    for i in range(target_orgs):
        name = f"Org{i}"
        if name in by_name:
            continue
        payload = {"name": name, "address": f"Main Street {i}"}
        status, created = http_json(base_url, "POST", "/api/v1/organizations", payload)
        if status not in (200, 201):
            raise RuntimeError(f"Failed to create organization {name}, status={status}")
        by_name[name] = created

    # Ensure stable mapping
    return {name: by_name[name]["id"] for name in by_name}


def ensure_users(base_url: str, target_users: int, target_orgs: int, first_name_term: str, last_name_term: str, organization_term: str):
    status, existing = http_json(base_url, "GET", "/api/v1/users")
    if status != 200:
        raise RuntimeError(f"Failed to list users, status={status}")

    existing = existing or []
    org_name_to_id = ensure_organizations(base_url, target_orgs)

    current_count = len(existing)
    if current_count >= target_users:
        return

    # Create missing users
    for i in range(current_count + 1, target_users + 1):
        org_idx = (i - 1) % target_orgs
        org_id = org_name_to_id[f"Org{org_idx}"]

        # Keep a predictable pattern for full-text search terms.
        payload = {
            "first_name": f"FirstName{i % 100}",
            "last_name": f"LastName{i % 100}",
            "title": "Engineer",
            # Always generate unique credentials to avoid collisions
            # from previous partially-created runs.
            "email": f"user{i}_{uuid.uuid4().hex[:8]}@example.com",
            "login": f"login{i}_{uuid.uuid4().hex[:8]}",
            "password": "password",
            "organization_id": org_id,
        }
        st = None
        for attempt in range(2):
            st, _ = http_json(base_url, "POST", "/api/v1/users", payload)
            if st in (200, 201):
                break
            if attempt == 0:
                # Retry once if server reports unique constraint collisions for any reason.
                payload["email"] = f"user{i}_{uuid.uuid4().hex[:8]}@example.com"
                payload["login"] = f"login{i}_{uuid.uuid4().hex[:8]}"
        if st not in (200, 201):
            raise RuntimeError(f"Failed to create user i={i}, status={st}")


def load_users(base_url: str):
    status, users = http_json(base_url, "GET", "/api/v1/users")
    if status != 200:
        raise RuntimeError(f"Failed to list users, status={status}")
    return users or []


def to_lua_string(s: str) -> str:
    # Use Lua long brackets to avoid escaping.
    # JSON doesn't contain ']]' normally.
    return f"[[{s}]]"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base-url", default="http://localhost:8080", help="Server base url (e.g. http://localhost:8080)")
    ap.add_argument("--target-users", type=int, default=10000)
    ap.add_argument("--target-orgs", type=int, default=50)
    ap.add_argument("--update-count", type=int, default=1000)
    ap.add_argument("--id-search-count", type=int, default=1000, help="How many user ids to include in id-search wrk script")
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--search-first-name", default="FirstName7")
    ap.add_argument("--search-last-name", default="LastName7")
    ap.add_argument("--search-organization", default="Org42")
    ap.add_argument("--out-dir", default=None, help="Where to write generated wrk lua scripts")
    args = ap.parse_args()

    base_url = args.base_url
    random.seed(args.seed)

    out_dir = args.out_dir or os.path.join(os.path.dirname(__file__), ".generated")
    os.makedirs(out_dir, exist_ok=True)

    ensure_users(
        base_url=base_url,
        target_users=args.target_users,
        target_orgs=args.target_orgs,
        first_name_term=args.search_first_name,
        last_name_term=args.search_last_name,
        organization_term=args.search_organization,
    )


    
    users = load_users(base_url)
    if len(users) < args.target_users:
        raise RuntimeError(f"Users after preparation are fewer than target: {len(users)} < {args.target_users}")

    user_ids = [u["id"] for u in users if u.get("id") is not None]
    if len(user_ids) < args.update_count:
        args.update_count = len(user_ids)

    update_ids = random.sample(user_ids, args.update_count)

    users_by_id = {u["id"]: u for u in users}

    id_search_count = min(args.id_search_count, len(user_ids))
    id_search_ids = random.sample(user_ids, id_search_count) if id_search_count > 0 else []
    id_search_paths = [f"/api/v1/users/{uid}" for uid in id_search_ids]

    updates = []
    for idx, uid in enumerate(update_ids):
        u = users_by_id[uid]
        # Update only non-unique fields to avoid collisions with unique constraints.
        new_title = f"UpdatedTitle{idx % 10}"
        new_password = f"password-upd-{idx}"
        payload = {
            "first_name": u["first_name"],
            "last_name": u["last_name"],
            "title": new_title,
            "email": u["email"],
            "login": u["login"],
            "password": new_password,
            "organization_id": u.get("organization_id", None),
        }
        updates.append({"id": uid, "body": json.dumps(payload, ensure_ascii=False)})

    updates_lua_path = os.path.join(out_dir, "updates.lua")
    with open(updates_lua_path, "w", encoding="utf-8") as f:
        f.write("local updates = {\n")
        for u in updates:
            f.write("  { id = %d, body = %s },\n" % (u["id"], to_lua_string(u["body"])))
        f.write("}\n")
        f.write(
            """
local idx = 1
request = function()
  local u = updates[idx]
  idx = idx + 1
  if idx > #updates then idx = 1 end
  wrk.method = "PUT"
  wrk.headers["Content-Type"] = "application/json"
  wrk.body = u.body
  return wrk.format("PUT", "/api/v1/users/" .. u.id)
end
"""
        )

    # Search phase scripts: alternate query variants.
    fn = args.search_first_name
    ln = args.search_last_name
    org = args.search_organization
    search_paths = [
        # f"/api/v1/users?first_name={fn}",
        # f"/api/v1/users?last_name={ln}",
        # f"/api/v1/users?organization={org}",
        # f"/api/v1/users?first_name={fn}&last_name={ln}",
        f"/api/v1/users?first_name={fn}&organization={org}",
    ]

    search_lua_path = os.path.join(out_dir, "search.lua")
    with open(search_lua_path, "w", encoding="utf-8") as f:
        f.write("local paths = {\n")
        for p in search_paths:
            f.write(f"  {to_lua_string(p)},\n")
        f.write(
            "}\nlocal idx = 1\nrequest = function()\n"
            "  local p = paths[idx]\n"
            "  idx = idx + 1\n"
            "  if idx > #paths then idx = 1 end\n"
            "  return wrk.format(\"GET\", p)\n"
            "end\n"
        )

    id_search_lua_path = os.path.join(out_dir, "id_search.lua")
    with open(id_search_lua_path, "w", encoding="utf-8") as f:
        f.write("local paths = {\n")
        for p in id_search_paths:
            f.write(f"  {to_lua_string(p)},\n")
        f.write(
            "}\nlocal idx = 1\nrequest = function()\n"
            "  local p = paths[idx]\n"
            "  idx = idx + 1\n"
            "  if idx > #paths then idx = 1 end\n"
            "  return wrk.format(\"GET\", p)\n"
            "end\n"
        )

    print("Generated wrk scripts:")
    print(updates_lua_path)
    print(search_lua_path)
    print(id_search_lua_path)


if __name__ == "__main__":
    main()

