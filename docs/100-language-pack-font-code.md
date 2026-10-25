# Origem do código de idioma nos pacotes de tradução

Data: 2026-10-25. Capítulo 4/M3. Continuação estática do relatório99; nenhum pacote foi aplicado em execução.

## Produtores confirmados

Language.SetLanguagePack14013 (0x1b17be4) possui caminho para KAIRO_JAPANESE e KAIRO_ENGLISH, campos estáticos+0x8/+0x10. Nesse caminho, escreve langPackFile_+0xc0 e escolhe `ja-JP` ou `en-US` para langPackCode_+0xa0; store em0x1b18084. Em seguida chama LoadLanguagePack14015 com argumentos nulos (0x1b181a4) e ClearCache14060. Portanto o código não é apenas um fallback do getter: existem atribuições explícitas.

LoadLanguagePack14015 (0x1b20554) também inicializa código/título/autor/escala antes de processar dados. Em0x1b20c8c, STP escreve autor em+0x98 e `en-US` em+0xa0; quando langPackFile_ é KAIRO_JAPANESE, troca para `ja-JP` em0x1b20ce4. A busca apenas por STR em+0xa0 teria omitido a primeira escrita.

O método guarda valores anteriores na pilha. Existe restauração do código em0x1b2582c, em uma região de tratamento de exceções. O alcance exato desse rollback e os tipos capturados não foram fechados por LSDA neste ciclo; não afirmar que toda falha restaura todo o estado.

## Diretiva do arquivo

Após separar chave/valor e tratar aspas externas, o carregador compara a chave com `@language` (GOT0x2739bc8). Se corresponde, chama Regex.Replace39453 (0x22b3a40) sobre o valor, com padrão `[^a-zA-Z\-]` (GOT0x2739b80) e substituição vazia (GOT0x272ee88). Armazena o resultado em langPackCode_ em0x1b211bc.

Depois, para código não nulo/vazio, divide por `-`. Somente quando há exatamente dois componentes, aplica ToLower617 ao primeiro, ToUpper620 ao segundo e concatena com hífen, gravando em0x1b22594. Um código de um componente, como `ZH`, não é convertido nessa etapa. A seleção posterior da fonte aplica seu próprio ToLower/Trim, conforme99.

O filtro é comprovadamente limitado a letras ASCII e hífen. A conversão de caixa usa métodos String com a cultura corrente; o código não justifica substituir toda a normalização por uma transformação de locale genérica. Não há neste trecho mapeamento `en-US`→`en-switch`.

## Dados locais conferidos

Os três CSVs do contêiner extraído `29_language` têm diretiva na linha2. Todos decodificaram integralmente com UTF-8; os arquivos SC/TC têm BOM UTF-8. Isso verifica esses arquivos, não todos os inputs aceitos pelo carregador.

| Arquivo local | Diretiva | SHA-256 |
| --- | --- | --- |
| 0000_Wizardquest_ko.csv | @language,ko | 6245c4c27129dbddbd1233dedf75bf3d0ac80ad559793b217588c3eff857523c |
| 0001_Wizardquest_SC.csv | @language,zh-CN | e1add22970f12ce17155a1ba97e1cdca7cdd7f3ca03e5f51b796d2dc8b2ae02c |
| 0002_WizardQuest_TC.csv | @language,ZH | bffb38c214f33c4291280ad036d1f746ac0c4464c48813172b4c873bd5ca8059 |

Esses dados fornecem códigos possíveis caso os respectivos pacotes sejam carregados. Não demonstram o idioma selecionado por uma instalação. Nenhum dos três declara `en-switch`, e o catálogo ResourceManager do relatório99 não contém fontes específicas `ko`, `zh` ou `zh-cn`. O fallback interno da Unity continua uma questão separada.

## Ligação ao boot e limite atual

Language.Init13986 lê PlayerPrefs.GetString a partir de PREF_LANG_PACK estático+0xe8 e passa o resultado a SetLanguagePack em0x1b16594. Há um caminho de tentativa com argumentos nulos após retorno false. SetLanguagePack também tem chamadas condicionais a PlayerPrefs.SetString/Save. Isso prova participação de configuração persistida; não revela seu valor numa instalação nem fecha todas as decisões anteriores de Init.

Nenhuma preferência/salvamento do usuário foi lida ou alterada. A chamada real do boot a Language.Init, seus caminhos de configuração e os argumentos que chegam à fonte das falas ainda precisam ser ligados. Não houve implementação de backend ou escolha aproximada de fonte neste ciclo.

## Evidências e próximo passo

- APK/ELF/metadados são os já inventariados; APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.
- `build/research/language-font-origin/`: cinco métodos exportados; cobertura sintática integral de37052 bytes, sem alegação de compreensão integral ou execução. TSV e log preservados. Assembly possui base adicional0x100000; VAs deste relatório não.
- Literais e offsets conferidos em `il2cpp-metadata-inventory.json` e ELF original. `pack-directives.json` local contém caminhos, hashes e diretivas; conteúdo proprietário permanece fora do Git.
- Sem alteração C++; testes de produto não repetidos. Último build registrado permanece18/18.

Confiança **confirmed** para stores, sanitização e dados locais; **unknown** para idioma efetivo e fallback de fonte. Próximo: conectar Language.Init ao boot e investigar os argumentos da fonte das falas; a pesquisa estática ainda permite avançar.
