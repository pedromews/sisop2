# PIX-sim (Atualizado)

## Requisitos
- CMake (versão 3.5 ou superior)
- Compilador C++ com suporte a C++17
- Make ou Ninja (sistema de build)

## Compilação

### Usando Make
```bash
mkdir build
cd build
cmake ..
make
```

### Usando Ninja (opcional, geralmente mais rápido)
```bash
mkdir build
cd build
cmake -G Ninja ..
ninja
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

## Testando em Máquinas Separadas
1. Compile o projeto em uma máquina.
2. Copie os executáveis `servidor` e `cliente` para a outra máquina.
3. Execute o servidor em uma máquina: `./servidor 4000`
4. Execute o cliente na outra máquina: `./cliente 4000`
5. No cliente, use o IP da máquina do servidor para as transações.

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

5. Para enviar transações entre clientes ou para o servidor, use os seguintes formatos:
   - Para enviar para o servidor: `server <valor>`
   - Para enviar para outro cliente: `<IP_do_cliente> <valor>`

   Exemplo:
   ```
   server 10
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
- Anote os IPs dos clientes para usar nas transações entre eles.
- Você pode enviar dinheiro do servidor para um cliente usando `server <valor>` e depois usar o IP desse cliente para enviar para outro.

Nota: Os arquivos Docker (Dockerfile e docker-compose.yml) estão localizados na pasta 'docker' dentro do diretório do projeto.

## Executando o Teste de Estresse

Para executar o teste de estresse que simula múltiplas transações concorrentes, siga estes passos:

1. Certifique-se de que os containers Docker estão em execução:
   ```
   docker-compose -f docker/docker-compose.yml up -d
   ```

2. Instale as dependências necessárias para o script de teste:
   ```
   pip install subprocess32
   ```

3. Execute o script de teste de estresse:
   ```
   python stress_test.py
   ```

Este script realizará 10.000 transações distribuídas entre três clientes, incluindo cenários de fundos insuficientes. Ao final do teste, você verá um resumo com o número total de transações, o tempo total de execução e o número de transações por segundo.

Nota: Certifique-se de que o Python está instalado em seu sistema antes de executar o script de teste de estresse.
