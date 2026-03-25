local paths = {
  [[/api/v1/users?first_name=FirstName7&organization=Org42]],
}
local idx = 1
request = function()
  local p = paths[idx]
  idx = idx + 1
  if idx > #paths then idx = 1 end
  return wrk.format("GET", p)
end
