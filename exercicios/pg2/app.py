from flask import Flask, jsonify, request, abort, make_response

app = Flask(__name__)

# -------- Hello, Echo  ja implementados --------

# 200 = Status OK

@app.get("/hello")
def hello():
    return jsonify(message="Olá, UFRGS!"), 200

@app.get("/echo/<msg>")
def echo_get(msg):
    # Retorna o que recebeu na URI
    return jsonify(echo=msg), 200

@app.post("/echo")
def echo_post():
    # Retorna o que recebeu no corpo JSON {"msg": "..."}
    data = request.get_json(silent=True) or {}
    msg = data.get("msg")
    if msg is None:
        abort(400, description="JSON invalido: esperado {'msg': <str>}")
    return jsonify(echo=str(msg)), 201


# -------- GET: Soma e Substracao --------

@app.get("/calculadora/somar/<int:a>/<int:b>")
def somar(a, b):
    soma = a + b
    retorno = {
        "operacao": "soma",
        "a": a,
        "b": b,
        "resultado": soma
    }
    return jsonify(retorno), 200


@app.get("/calculadora/subtrair/<int:a>/<int:b>")
def subtrair(a, b):
    subtracao = a - b
    retorno = {
        "operacao": "subtracao",
        "a": a,
        "b": b,
        "resultado": subtracao
    }
    return jsonify(retorno), 200

#--------- POST: multiplicar e dividir ----

@app.post("/calculadora/multiplicar")
def multiplicar():
    data = request.get_json(silent=True) or {}
    a = data.get("a")
    b = data.get("b")

    if a is None:
        abort(400, description="JSON invalido: esperado {'a': int}")
    if b is None:
        abort(400, description="JSON invalido: esperado {'b': int}")

    try:
        a = int(a)
        b = int(b)
    except (TypeError, ValueError):
        abort(400, description="'a' e 'b' devem ser inteiros")

    multiplicacao = a * b
    retorno = {
        "operacao": "multiplicar",
        "a": a,
        "b": b,
        "resultado": multiplicacao
    }
    return jsonify(retorno), 201


@app.post("/calculadora/dividir")
def dividir():
    data = request.get_json(silent=True) or {}
    a = data.get("a")
    b = data.get("b")

    if a is None:
        abort(400, description="JSON invalido: esperado {'a': int}")
    if b is None:
        abort(400, description="JSON invalido: esperado {'b': int}")

    try:
        a = int(a)
        b = int(b)
    except (TypeError, ValueError):
        abort(400, description="'a' e 'b' devem ser inteiros")

    if b == 0:
        abort(400, description="Divisao por zero nao eh permitida")

    divisao = a / b
    retorno = {
        "operacao": "dividir",
        "a": a,
        "b": b,
        "resultado": divisao
    }
    return jsonify(retorno), 201

# -------- Tratamento global de erros --------

@app.errorhandler(400)
def bad_request(error):
    """Retorna erros 400 em formato JSON, em vez de HTML."""
    response = jsonify({
        "erro": "Bad Request",
        "mensagem": error.description
    })
    return make_response(response, 400)

# -------- Main --------

if __name__ == "__main__":
    app.run(host="127.0.0.1", port=8000, debug=True)