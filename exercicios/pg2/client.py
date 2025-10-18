import requests

BASE = "http://127.0.0.1:8000"

def ok(r):
    r.raise_for_status()
    return r

def main():

    # ---- Echo 
    print("Echo GET:", ok(requests.get(f"{BASE}/echo/hello")).json())
    print("Echo POST:", ok(requests.post(f"{BASE}/echo", json={"msg": "Hello, UFRGS!"})).json())

    # ---- GET: soma 
    print("GET soma 2+3:", ok(requests.get(f"{BASE}/calculadora/somar/2/3")).json())

    # ---- POST: multiplicação e divisão ----
    r = ok(requests.post(f"{BASE}/calculadora/multiplicar", json={"a": 4, "b": 5}))
    print("POST multiplicar 4*5:", r.json())
    
    r = ok(requests.post(f"{BASE}/calculadora/dividir", json={"a": 20, "b": 4}))
    print("POST dividir 20/4:", r.json())
    
    # ---- GET: subtração
    r = requests.get(f"{BASE}/calculadora/subtrair/9/2")
    print("GET subtrair 9-2 ->", r.status_code, r.json())
    
    # ---- POST: divisão por zero (teste de erro) ----
    r = requests.post(f"{BASE}/calculadora/dividir", json={"a": 10, "b": 0})
    if r.status_code == 400:
        print("POST dividir 10/0: erro esperado ->", r.status_code, r.text)
    else:
        print("POST dividir 10/0:", r.status_code, r.json())

if __name__ == "__main__":
    main()
