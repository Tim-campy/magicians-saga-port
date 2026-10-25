# Preparação dos textos de fala e de evento

Data: 2026-10-25. Evidência estática ARM64 e auditoria de arquivos locais. VAs sem base Ghidra0x100000. Sem execução do jogo.

## Tipos e preparação das falas

Metadata dos parâmetros de StringUtil.Replace14531 e SubForm.MakeTalkForm36177 confirma IL2CPP_TYPE_STRING (14) para texto e args. Portanto args não é um array genérico de argumentos: é uma string codificando campos separados por TAB. A fronteira nativa EventObject ainda é opaca; sua futura implementação deve fornecer esse tipo, não presumir formato de .NET String.Format.

MakeTalkForm36175 lê TalkData.charaIndex_ (+0x20) e AppData.talks_[id] (campo estático+0x378), depois chama o overload36177 em0x1204bc4. Este:

1. Aplica StringUtil.Replace quando args não é null.
2. Cria SubForm; guarda args em talkArg_+0x218, type_=0, x_/y_=0, select_=0, humanCharaIndex_=argumento e nextIndex_=-1.
3. Divide texto por TAB usando Split14533 e guarda textAry_+0x1f8; textIdx_=0.
4. Cria TextLayout, define tamanho196×60 e SetSpeed(1500), além de cor/parâmetros; fornece o primeiro texto e chama SetTextCheckSize(-1).

Esses números são parâmetros das chamadas observadas; unidade de velocidade, desenho, paginação e coordenadas finais não estão demonstrados. A leitura do primeiro texto falha se não houver elemento; não criar página fictícia para texto ausente.

## Mensagens de evento

MakeEvent36178, VA0x1205028, lê EventMsgData.type (+0x20) e text (+0x28), concatena a representação decimal do tipo, o literal `&` e o texto, e encaminha ao overload36179 em0x1205120.

O overload aplica StringUtil.Replace, divide por TAB com Split14533, aloca eventIds_ e eventTexts_ de mesmo comprimento e, para cada segmento, divide por `&` usando flags0/0/0. Faz ParseInt do elemento0 e usa elemento1 como texto; campos posteriores não são usados nesse laço. Ausência do segundo campo falha. Não tratar isso como separação apenas no primeiro `&` preservando todo o restante.

O formulário resultante recebe type_=4, x_/y_=0, select_=0 e eventAnim_ com quatro inteiros inicialmente zero. Esses campos e os nomes anteriores foram conferidos na metadata/tabela de offsets; ainda falta comportamento de atualização e desenho desse tipo.

## Split e substituição

Split14533 em0x1c195ec encaminha à implementação14535 com flags(1,0,0): aplica trim, preserva segmentos vazios e não habilita tratamento de escape. Diferencia-se do parser de tabelas/inteiros do relatório76, que usa flags(0,0,0). Semântica completa do trim Unicode continua pendente; não implementar com strip/split genéricos e declarar equivalência.

StringUtil.Replace14531, VA0x1c18df4, retorna o texto original se texto/args é null ou args tem comprimento zero. Caso contrário:

- Divide args por TAB com flags(0,0,0), preservando campos vazios.
- Passa cada campo por Language.TranslateText14007 com segundo argumento0.
- Constrói o marcador `<índice>` e substitui suas ocorrências pelo campo traduzido.
- A substituição literal14532 avança a busca para depois do texto inserido; não reprocessa a mesma inserção indefinidamente. Marcadores de índices posteriores inseridos por um argumento podem ser alcançados nas passagens seguintes.

Existe tratamento condicional do padrão `(さん(</co>)?)さん`, com replacement `$1`. Quando o backing field estático StringUtil.SanSanRemove é verdadeiro, esse ramo de limpeza é pulado. Quando falso, só aplica Regex.Replace após a substituição se nem o texto anterior nem o argumento traduzido já correspondiam ao padrão. Isso distingue duplicação introduzida pela substituição de duplicação preexistente. Não inverter a condição com base no nome da propriedade. Valor inicial, escritores dessa propriedade e efeitos de TranslateText ainda precisam ser fechados.

Literais conferidos diretamente pela metadata: TAB em GOT0x272feb8, `&` em0x2732430, padrão em0x273fed0, delimitadores de marcador em0x2739e50/0x2739da0 e replacement em0x2739fe0. Exports locais: form-data-preparation, form-text-helpers e form-replace-core.

## Formatos das tabelas e auditoria reproduzível

EventMsgData.Load38322 em0x116fc90 lê id, tipo e um token string. TalkData.Load38544 em0x117347c lê id, charaIndex_ e um **array de strings com contagem**, não um texto único. GetStringArray36583 em0x1127384 lê contagem assinada: negativa retorna null; zero produz array vazio; positiva lê exatamente essa quantidade de tokens. Ambos os loaders toleram linha null/vazia retornando o objeto sem carregar campos; a ferramenta abaixo audita linhas não vazias dos arquivos, não emula construção de objetos.

Ferramenta `tools/inspect_presentation_table.py` preserva CR e tokens extras, aceita o subconjunto decimal/whitespace ASCII de Int32.Parse e mantém null/vazio distintos. Não traduz nem seleciona idioma.

```powershell
python tools/inspect_presentation_table.py talk data/extracted/members/06_xls/0017_English.lproj_Talk.txt --output build/research/talk-table.json
python tools/inspect_presentation_table.py event data/extracted/members/06_xls/0008_English.lproj_EventMsg.txt --output build/research/eventmsg-table.json
python -m unittest discover -s tests -p test_presentation_table.py
```

Auditoria nos dois idiomas, artefatos em `build/research/presentation-tables/`:

| Tabela | Registros por idioma | IDs | Duplicatas / tokens extras / arrays null |
| --- | --- | --- | --- |
| Talk.txt | 46 | 0–45 | Nenhum |
| EventMsg.txt | 24 | 0–23 | Nenhum |

As57 chamadas de opcode8 referem45 IDs distintos; as22 de opcode12 referem22 IDs distintos. Nenhum ID está ausente das tabelas inglesas auditadas. Isso verifica referências estáticas dos scripts extraídos, não alcance runtime, idioma efetivo ou completude de apresentação.

Cinco testes Python passaram, cobrindo contagens, null/vazio, CR, campos extras, limites e entradas truncadas. Não houve alteração C++ ou repetição de build; último CTest Windows permanece13/13 (`84`).

## Próximo passo

Rastrear o produtor de AppData.talks_ e a transformação do array TalkData.text em texto do formulário. Fechar TranslateText, flag de limpeza e trim antes do parser/modelo nativo de apresentação. A pesquisa estática segue útil; não há necessidade de teste dinâmico como único avanço neste ponto.
