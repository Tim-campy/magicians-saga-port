# Metadados IL2CPP para endereços ARM64

Data: 2026-10-25. Mapeamento estático de tabelas, relocations ELF e tokens. Não executa `libil2cpp.so`, não decompila métodos e não afirma comportamento de gameplay.

## Entradas e reprodução

- APK SHA-256: `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.
- Membro: `lib/arm64-v8a/libil2cpp.so`, 42.888.064 bytes.
- ELF SHA-256: `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`.
- Metadata SHA-256: `8f79c53ce22e97269f7ac7dd932cd4d80ed83169a5a61f9b9a85fc26d08a0cbd`.
- Entrada de nomes/protótipos: `research/local/il2cpp-metadata-inventory.json`, produzido pelo parser v31 descrito em `05-il2cpp-metadata.md`.

```powershell
python tools/extract_research_members.py "Magician's+Saga_1.4.0_APKPure.apk" --output build/research/il2cpp --member lib/arm64-v8a/libil2cpp.so
python tools/map_il2cpp_methods.py build/research/il2cpp/lib/arm64-v8a/libil2cpp.so research/local/il2cpp-metadata-inventory.json --output research/local/il2cpp-code-map.json
```

Extração específica com manifesto separado em `build/research/il2cpp/extraction-manifest.json`. Biblioteca e catálogo permanecem locais e ignorados pelo Git.

## Fontes do layout

O script aplica o início e o tamanho ARM64 v31 de `Il2CppCodeGenModule` e o lookup por token de [Il2CppDumper Il2CppClass.cs](https://github.com/Perfare/Il2CppDumper/blob/4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/Il2CppClass.cs) e [Il2Cpp.cs/GetMethodPointer](https://github.com/Perfare/Il2CppDumper/blob/4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/Il2Cpp.cs), ambos fixados no commit `4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a`. A posição do método na tabela é `(token & 0x00ffffff) - 1`; não é o índice global do metadata.

Relocations seguem a [ABI ELF AArch64 da Arm](https://github.com/ARM-software/abi-aa/blob/main/aaelf64/aaelf64.rst#dynamic-relocations): RELATIVE (1027) ajusta o addend pela base de carregamento; ABS64 (257) soma símbolo e addend. O relatório fixa load bias em zero. **Endereços são VAs ELF, não endereços observados em execução nem offsets de arquivo.**

## Cadeia de validação

1. Parser exige ELF64 little-endian, `ET_DYN`, máquina 183/AArch64 e valida cabeçalhos/intervalos PT_LOAD e RELA.
2. Identifica strings exatas dos 64 nomes de images do metadata e segue relocations RELATIVE que apontam para elas.
3. Valida estruturas candidatas: nome, contagem de métodos idêntica ao metadata, tabela de ponteiros dentro de PT_LOAD e tokens RID únicos cobrindo `1..count` por image.
4. Resolve cada slot via relocation. Todo ponteiro não nulo precisa estar alinhado a quatro bytes e apontar para bytes de um segmento ELF executável. Slots zero permanecem desconhecidos. Tabelas inteiramente em BSS são tratadas como memória inicialmente zero, não como bytes presentes no arquivo.
5. Corrobora as 64 estruturas em um array completo e exclusivo, referenciado por um ponteiro precedido da contagem 64 no layout da cauda de `Il2CppCodeRegistration`.

Esse encadeamento é mais forte que coincidência de strings. Ainda é inferência estrutural: a inicialização que entrega o registro ao runtime não foi analisada nesta tarefa.

## Registro e módulos

| Elemento | VA ELF |
|---|---|
| Base candidata de `Il2CppCodeRegistration` | `0x2604920` |
| Campo `codeGenModulesCount` (=64) | `0x2604998` |
| Campo `codeGenModules` | `0x26049a0` |
| Array de ponteiros dos 64 módulos | `0x27a0798` |
| Módulo `Assembly-CSharp.dll` | `0x2625400` |
| Nome `Assembly-CSharp.dll` | `0x6375a9` |
| Tabela de seus 4.704 métodos | `0x27a89e8` |
| Módulo `KairoLibrary.dll` | `0x2708df8` |
| Tabela de seus 6.771 métodos | `0x28740c8` |

A base de CodeRegistration é **probable**, inferida pela cauda; seus outros campos não foram validados. A correspondência nome/contagem/slots dos módulos é **confirmed sob o schema aplicado**, não uma captura do runtime.

## Primeiros callbacks da lógica gerenciada compilada

Tipo IL2CPP `main.Main`, index 4166, em `Assembly-CSharp.dll`; separado do DEX `main.Main`.

| Método | Metadata index | Token | Slot VA | Função VA ELF | Offset no ELF |
|---|---:|---|---|---|---|
| `OnCreate` | 34537 | `0x0600026c` | `0x27a9d40` | `0x117c78c` | `0x117878c` |
| `OnUpdate` | 34538 | `0x0600026d` | `0x27a9d48` | `0x117cdc8` | `0x1178dc8` |
| `OnDraw` | 34539 | `0x0600026e` | `0x27a9d50` | `0x117db20` | `0x1179b20` |

Os três slots têm relocations tipo 1027, símbolo 0. Registros RELA no arquivo: `0x45a868`, `0x45a880` e `0x45a898`, respectivamente. Seus addends são exatamente as VAs de função acima. Os bytes de cada entrada estão em segmento executável e têm alinhamento ARM64.

Próximas VAs distintas conhecidas: `OnCreate` → `0x117cdc8`, `OnUpdate` → `0x117da00`, `OnDraw` → `0x117dbe0`. Isso oferece apenas um limite de busca, **não estabelece o final do corpo de uma função**: helpers genéricos, thunks ou funções não mapeadas podem estar entre essas entradas. Decompilação exige limites obtidos de disassembly/controle de fluxo ou informações de unwind.

## Cobertura e limites

O JSON contém os **63.273 métodos** de todas as 64 images: **56.955 ponteiros não nulos para VAs executáveis distintas** e **6.318 slots nulos não resolvidos**. Nos três módulos candidatos do jogo há 11.690 registros, sendo 11.174 não nulos. Não atribuir uma causa a cada slot nulo sem analisar flags/genéricos; métodos abstratos e especializações não foram classificados aqui.

`mapped_methods` registra `method_index`, token, owner, nome de image, slot, relocation, VA, offset e `next_distinct_mapped_method_va`. `priority_callbacks` destaca os três callbacks. `modules` registra todos os módulos e `code_registration_candidate` a referência corroborante. Há 257.114 relocations RELATIVE, 1.631 ABS64, 565 JUMP_SLOT e 153 GLOB_DAT no ELF; o resolvedor de slots admite RELATIVE e ABS64 com símbolo definido, e rejeita ponteiros não nulos sem relocation suportada.

Não foram resolvidos registration de tipos, instâncias genéricas, limites de funções, regras de gameplay, áudio, saves ou RNG. Nenhum endereço foi observado no processo Android. Ao importar em Ghidra, aplicar explicitamente qualquer rebase do projeto; não confundir o image base escolhido pelo Ghidra com as VAs deste relatório.

## Validações executadas

- Duas execuções produziram estruturas idênticas; igualdade com o JSON salvo confirmada.
- Seis entradas inválidas rejeitadas: três truncamentos ELF, arquitetura errada, relocation de callback redirecionada para string não executável e RID duplicado.
- Hash da biblioteca extraída permaneceu igual.
- Teste inicial revelou tabela de seis slots de `UnityEngine.Purchasing.AppleCore.dll` em BSS (`0x28fce50`); parser foi corrigido para representar os zeros de memória do ELF, sem fingir bytes de arquivo ou endereços recuperados.

Próxima tarefa delimitada: importar/seedar os três callbacks e entradas adjacentes no Ghidra, verificar limites de função e seguir uma camada de chamadas de `OnUpdate`. A confirmação do registro em execução e a comparação com o jogo continuam pendentes.
