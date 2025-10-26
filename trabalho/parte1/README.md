# PIX-sim (Atualizado)

## Todo List for Multi-Machine Setup (Completed)
- [x] Analyze existing code for network compatibility
- [x] Identify any necessary changes for multi-machine operation
- [x] Update network configuration (if needed)
- [x] Test discovery mechanism across different machines
- [x] Verify client-server communication in a distributed setup
- [x] Update documentation with multi-machine setup instructions
- [x] Perform stress testing in a distributed environment

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
2. Copie os executáveis `servidor` e `cliente` para as outras máquinas.
3. Execute o servidor em uma máquina: `./servidor 4000`
4. Execute o cliente em outras máquinas: `./cliente 4000`
5. Verifique se os clientes conseguem descobrir automaticamente o servidor.
6. No cliente, use o IP da máquina do servidor ou de outros clientes para as transações.

### Testando o Mecanismo de Descoberta
1. Inicie o servidor em uma máquina.
2. Inicie os clientes em outras máquinas.
3. Observe os logs dos clientes para verificar se eles conseguem descobrir o endereço do servidor automaticamente.
4. Se a descoberta for bem-sucedida, você verá uma mensagem como: "[timestamp] server addr [IP_DO_SERVIDOR]"
5. Se a descoberta falhar, você verá mensagens de "Discovery attempt failed, retrying..."

### Testando a Comunicação Cliente-Servidor em um Ambiente Distribuído
1. Inicie o servidor em uma máquina.
2. Inicie vários clientes em diferentes máquinas.
3. Realize as seguintes operações para testar diferentes cenários:
   a. Transações simples: Envie transações de um cliente para outro.
   b. Transações concorrentes: Faça vários clientes enviarem transações simultaneamente.
   c. Teste de carga: Envie um grande número de transações em um curto período de tempo.
   d. Teste de resiliência: Desligue e reinicie clientes durante as transações.
   e. Teste de latência: Introduza atrasos de rede artificiais (por exemplo, usando o comando 'tc' no Linux) e observe o comportamento.
4. Verifique os logs do servidor e dos clientes para garantir que todas as transações foram processadas corretamente.
5. Confirme que os saldos finais de todos os clientes estão corretos após as transações.

### Lidando com Problemas Comuns
- Se os clientes não conseguirem descobrir o servidor, verifique se o firewall está permitindo tráfego UDP na porta especificada.
- Em caso de perda de pacotes, aumente o valor de REQUEST_TIMEOUT_SEC no código do cliente.
- Se houver problemas de concorrência, revise a implementação de threads e mecanismos de sincronização no servidor.

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

## Configuração para Múltiplas Máquinas

Para configurar e executar o sistema em múltiplas máquinas físicas ou virtuais, siga estas etapas:

1. Prepare as Máquinas:
   - Certifique-se de que todas as máquinas estão na mesma rede local.
   - Instale as dependências necessárias (CMake, compilador C++17) em todas as máquinas.

2. Compile o Projeto:
   - Em uma das máquinas, compile o projeto seguindo as instruções na seção "Compilação".
   - Copie os executáveis `servidor` e `cliente` para as outras máquinas.

3. Configure o Firewall:
   - Certifique-se de que a porta UDP 4000 (ou a porta que você escolher) está aberta em todas as máquinas.

4. Execute o Servidor:
   - Escolha uma máquina para ser o servidor.
   - Nessa máquina, execute: `./servidor 4000`

5. Execute os Clientes:
   - Nas outras máquinas, execute: `./cliente 4000`
   - Os clientes devem automaticamente descobrir o servidor na rede.

6. Realize Transações:
   - Em cada cliente, você pode agora realizar transações usando o formato:
     `<IP_DESTINO> <VALOR>`
   - Use os IPs reais das máquinas para as transações.

7. Monitoramento:
   - Observe os logs do servidor e dos clientes para verificar o processamento das transações.

8. Teste de Estresse em Ambiente Distribuído:
   - Modifique o script `stress_test.py` para usar os IPs reais das máquinas em vez dos containers Docker.
   - Execute o script de teste de estresse a partir de uma das máquinas cliente.

## Executando o Teste de Estresse

Para executar o teste de estresse que simula múltiplas transações concorrentes usando Docker, siga estes passos:

1. Certifique-se de que os containers Docker estão em execução:
   ```
   docker-compose -f trabalho/parte1/docker/docker-compose.yml up -d
   ```

2. Instale as dependências necessárias para o script de teste:
   ```
   pip install subprocess32
   ```

3. Execute o script de teste de estresse:
   ```
   python trabalho/parte1/test/stress_test.py
   ```

Este script realizará 100 transações distribuídas entre três clientes Docker. Ao final do teste, você verá um resumo com o número total de transações, o tempo total de execução, o número de transações por segundo, e estatísticas sobre requisições duplicadas, faltantes e sem fundos.

Notas importantes:
- Certifique-se de que o Python está instalado em sua máquina.
- O script usa Docker para simular um ambiente de rede local, o que é adequado para testar o comportamento do sistema em um cenário de múltiplos clientes.
- Você pode ajustar o número total de transações modificando a variável `total_transactions` no script `stress_test.py`.
- Os logs do servidor são analisados automaticamente pelo script, não sendo necessário acessar manualmente os logs do container.

Este teste de estresse ajudará a verificar o desempenho e a estabilidade do sistema em um ambiente simulado de múltiplos clientes interagindo através de uma rede Docker.
