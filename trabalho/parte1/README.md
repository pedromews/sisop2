# PIX-sim (atualizado)

## Compilação
```bash
mkdir build
cd build
cmake ..
make
```

## Execução
### Servidor
```bash
./servidor 4000
```

### Cliente
```bash
./cliente 4000
```
Comandos (stdin):
```
<IP_DESTINO> <VALOR>
ex: 10.1.1.3 10
```

Comportamento novo:
- Cliente: timeout e retransmissão (configurável via constantes), usa `select()` para aguardar ACK.
- Servidor: trata duplicadas e lacunas. Se receber `seqn` <= `last_req`, responde com ack informando último `seqn` processado e saldo. Se receber `seqn` > `last_req + 1`, responde com ack com último `seqn` processado (sem aplicar a transação).
- Arquivos usam `using namespace std;` conforme solicitado.
