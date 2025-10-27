# PIX Simulator

## Requisitos
- CMake (versão 3.5 ou superior)
- Compilador C++ com suporte a C++17
- Make

## Compilação

### Usando Make
```bash
rm -rf build && mkdir build
cd build
cmake ..
make
```

## Execução
### Servidor
```bash
./server 4000
```

### Cliente
```bash
./client 4000
```

Comandos (stdin):
```
<IP_DESTINO> <VALOR>
ex: 10.1.1.3 10
```

## Testando em Máquinas Separadas
1. Compile o projeto em uma máquina.
2. Copie os executáveis `server` e `client` para as outras máquinas.
3. Execute o servidor em uma máquina: `./server 4000`
4. Execute o cliente em outras máquinas: `./client 4000`
5. Verifique se os clientes conseguem descobrir automaticamente o servidor.
6. No cliente, use o IP da máquina do servidor ou de outros clientes para as transações.

## Limpeza
Para limpar os arquivos de build:
```bash
cd build
make clean
```
Ou simplesmente delete o diretório `build`.

## Controle de Versão
Um arquivo `.gitignore` foi adicionado ao projeto para evitar que arquivos de build e outros arquivos desnecessários sejam commitados. Certifique-se de usar este arquivo ao versionar o projeto.

## Comportamento
- Cliente: timeout e retransmissão (configurável via constantes), usa `select()` para aguardar ACK.
- Servidor: trata duplicadas e lacunas. Se receber `seqn` <= `last_req`, responde com ack informando último `seqn` processado e saldo. Se receber `seqn` > `last_req + 1`, responde com ack com último `seqn` processado (sem aplicar a transação).
- Arquivos usam `using namespace std;` conforme solicitado.

## Estrutura do Projeto
- `include/`: Arquivos de cabeçalho (.hpp)
- `src/`: Arquivos fonte (.cpp)
- `CMakeLists.txt`: Arquivo de configuração do CMake
- `.gitignore`: Lista de arquivos e diretórios ignorados pelo Git
- `Dockerfile`: Configuração para construir a imagem Docker
- `docker-compose.yml`: Configuração para executar múltiplos containers Docker

## Executando com Docker

Para simular uma rede local com um servidor e múltiplos clientes usando Docker, siga estas etapas:

1. Certifique-se de ter o Docker e o Docker Compose instalados em sua máquina.

2. Na pasta do projeto (trabalho/parte1), execute o seguinte comando para construir e iniciar os containers:

   ```
   docker-compose -f docker/docker-compose.yml up --build
   ```

   Isso iniciará um servidor e três clientes em containers separados.

3. Para interagir com os clientes, abra três terminais separados e execute os seguintes comandos em cada um:

   Terminal 1 (client1):
   ```
   docker-compose -f docker/docker-compose.yml exec -it client1 /bin/bash
   ```

   Terminal 2 (client2):
   ```
   docker-compose -f docker/docker-compose.yml exec -it client2 /bin/bash
   ```

   Terminal 3 (client3):
   ```
   docker-compose -f docker/docker-compose.yml exec -it client3 /bin/bash
   ```

4. Em cada terminal de cliente, inicie o cliente:
   ```
   ./build/client 4000
   ```

5. Para enviar transações entre clientes, use os seguintes formatos:
   - `<IP_do_cliente> <valor>`

   Exemplo:
   ```
   172.18.0.3 20
   ```

   Para obter os IPs dos clientes, você pode usar o comando `hostname -i` dentro do terminal de cada cliente.

6. Para ver os logs do servidor:
   ```
   docker-compose -f docker/docker-compose.yml logs -f server
   ```

7. Para parar e remover os containers, use:
   ```
   docker-compose -f docker/docker-compose.yml down
   ```

Dicas para testar:
- Use `hostname -i` em cada terminal de cliente para obter seu IP.
