import requests

url = "http://10.42.0.1:5001"
data = {"direction": 0}
# r = requests.post(url json=data)
r = requests.get(url + "/test")

print("Status", r.status_code)
print("Json", r.json())
