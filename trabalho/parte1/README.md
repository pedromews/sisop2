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
