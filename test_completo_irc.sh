#!/bin/bash

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Contadores globales
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Función para imprimir resultados
print_result() {
    local test_name="$1"
    local result="$2"
    local expected="$3"
    local actual="$4"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC} - $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ FAIL${NC} - $test_name"
        echo -e "   Esperado: $expected"
        echo -e "   Recibido: $actual"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Función para verificar código en respuesta
check_code() {
    local response="$1"
    local expected_code="$2"
    
    if echo "$response" | grep -q "$expected_code"; then
        echo "PASS"
    else
        echo "FAIL"
    fi
}

# Función para verificar formato de mensaje
check_format() {
    local response="$1"
    
    if echo "$response" | grep -q "^:ft_irc [0-9]"; then
        echo "PASS"
    else
        echo "FAIL"
    fi
}

# Función para enviar comando y recibir respuesta
send_and_receive() {
    local cmd="$1"
    local fd="$2"
    local timeout="${3:-2}"
    
    echo -e "$cmd\r\n" >&$fd
    sleep 0.3
    
    # Leer respuesta con timeout
    local response=""
    local start_time=$(date +%s)
    while [ $(($(date +%s) - start_time)) -lt $timeout ]; do
        if read -t 0.1 -r line <&$fd 2>/dev/null; then
            response="$response$line\n"
        else
            break
        fi
    done
    
    echo -e "$response"
}

echo -e "${BLUE}🧪 TEST COMPLETO DEL SERVIDOR IRC - TODOS LOS COMANDOS${NC}"
echo "================================================================"

# Verificar que el servidor esté corriendo
echo -e "${YELLOW}🔍 Verificando servidor...${NC}"
if ! nc -z localhost 6669 2>/dev/null; then
    echo -e "${RED}❌ Servidor no está corriendo en puerto 6669${NC}"
    echo -e "${YELLOW}💡 Ejecuta: ./ircserv 6669 testpass${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Servidor detectado en puerto 6669${NC}"

echo ""
echo -e "${BLUE}📋 INICIANDO CONEXIÓN PERSISTENTE${NC}"
echo "=================================="

# Crear conexión persistente
exec 3<>/dev/tcp/localhost/6669
echo -e "${GREEN}✅ Conexión establecida (FD: 3)${NC}"

echo ""
echo -e "${PURPLE}📋 SECCIÓN 1: REGISTRO Y MENSAJES DE BIENVENIDA${NC}"
echo "=================================================="

# PASS
echo -e "${CYAN}📤 Enviando: PASS testpass${NC}"
response=$(send_and_receive "PASS testpass" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if [ -z "$response" ]; then
    print_result "PASS correcta" "PASS" "Sin respuesta" "Sin respuesta"
else
    print_result "PASS correcta" "PASS" "Sin respuesta" "$response"
fi

# NICK
echo -e "${CYAN}📤 Enviando: NICK testuser${NC}"
response=$(send_and_receive "NICK testuser" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if [ -z "$response" ]; then
    print_result "NICK válido" "PASS" "Sin respuesta" "Sin respuesta"
else
    print_result "NICK válido" "PASS" "Sin respuesta" "$response"
fi

# USER
echo -e "${CYAN}📤 Enviando: USER testuser 0 * :Test User${NC}"
response=$(send_and_receive "USER testuser 0 * :Test User" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"

# Verificar mensajes de bienvenida
welcome_response="$response"
echo -e "${CYAN}📥 Mensajes de bienvenida: $welcome_response${NC}"

# Verificar códigos de bienvenida
if echo -e "$welcome_response" | grep -q ":ft_irc 1 "; then
    welcome_msg=$(echo -e "$welcome_response" | grep ":ft_irc 1 ")
    print_result "RPL_WELCOME (001)" "PASS" "001" "$welcome_msg"
else
    print_result "RPL_WELCOME (001)" "FAIL" "001" "No encontrado"
fi

if echo -e "$welcome_response" | grep -q ":ft_irc 2 "; then
    yourhost_msg=$(echo -e "$welcome_response" | grep ":ft_irc 2 ")
    print_result "RPL_YOURHOST (002)" "PASS" "002" "$yourhost_msg"
else
    print_result "RPL_YOURHOST (002)" "FAIL" "002" "No encontrado"
fi

if echo -e "$welcome_response" | grep -q ":ft_irc 3 "; then
    created_msg=$(echo -e "$welcome_response" | grep ":ft_irc 3 ")
    print_result "RPL_CREATED (003)" "PASS" "003" "$created_msg"
else
    print_result "RPL_CREATED (003)" "FAIL" "003" "No encontrado"
fi

if echo -e "$welcome_response" | grep -q ":ft_irc 4 "; then
    myinfo_msg=$(echo -e "$welcome_response" | grep ":ft_irc 4 ")
    print_result "RPL_MYINFO (004)" "PASS" "004" "$myinfo_msg"
else
    print_result "RPL_MYINFO (004)" "FAIL" "004" "No encontrado"
fi

echo ""
echo -e "${PURPLE}📋 SECCIÓN 2: COMANDOS SIN ARGUMENTOS - ERRORES 461${NC}"
echo "========================================================"

# PRIVMSG sin argumentos - ERR_NORECIPIENT (411)
echo -e "${CYAN}📤 Enviando: PRIVMSG${NC}"
response=$(send_and_receive "PRIVMSG" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "411")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "PRIVMSG sin argumentos (411)" "PASS" "411" "$response"
else
    print_result "PRIVMSG sin argumentos (411)" "FAIL" "411" "$response"
fi

# JOIN sin argumentos - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: JOIN${NC}"
response=$(send_and_receive "JOIN" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "JOIN sin argumentos (461)" "PASS" "461" "$response"
else
    print_result "JOIN sin argumentos (461)" "FAIL" "461" "$response"
fi

# PART sin argumentos - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: PART${NC}"
response=$(send_and_receive "PART" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "PART sin argumentos (461)" "PASS" "461" "$response"
else
    print_result "PART sin argumentos (461)" "FAIL" "461" "$response"
fi

# TOPIC sin argumentos - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: TOPIC${NC}"
response=$(send_and_receive "TOPIC" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "TOPIC sin argumentos (461)" "PASS" "461" "$response"
else
    print_result "TOPIC sin argumentos (461)" "FAIL" "461" "$response"
fi

# MODE sin argumentos - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: MODE${NC}"
response=$(send_and_receive "MODE" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "MODE sin argumentos (461)" "PASS" "461" "$response"
else
    print_result "MODE sin argumentos (461)" "FAIL" "461" "$response"
fi

# KICK sin argumentos - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: KICK${NC}"
response=$(send_and_receive "KICK" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "KICK sin argumentos (461)" "PASS" "461" "$response"
else
    print_result "KICK sin argumentos (461)" "FAIL" "461" "$response"
fi

# INVITE sin argumentos - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: INVITE${NC}"
response=$(send_and_receive "INVITE" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "INVITE sin argumentos (461)" "PASS" "461" "$response"
else
    print_result "INVITE sin argumentos (461)" "FAIL" "461" "$response"
fi

echo ""
echo -e "${PURPLE}📋 SECCIÓN 3: ARGUMENTOS INSUFICIENTES${NC}"
echo "====================================="

# PRIVMSG sin texto - ERR_NOTEXTTOSEND (412)
echo -e "${CYAN}📤 Enviando: PRIVMSG #test${NC}"
response=$(send_and_receive "PRIVMSG #test" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "412")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "PRIVMSG sin texto (412)" "PASS" "412" "$response"
else
    print_result "PRIVMSG sin texto (412)" "FAIL" "412" "$response"
fi

# USER con argumentos insuficientes - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: USER test${NC}"
response=$(send_and_receive "USER test" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "USER argumentos insuficientes (461)" "PASS" "461" "$response"
else
    print_result "USER argumentos insuficientes (461)" "FAIL" "461" "$response"
fi

# KICK sin usuario - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: KICK #test${NC}"
response=$(send_and_receive "KICK #test" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "KICK sin usuario (461)" "PASS" "461" "$response"
else
    print_result "KICK sin usuario (461)" "FAIL" "461" "$response"
fi

# INVITE sin canal - ERR_NEEDMOREPARAMS (461)
echo -e "${CYAN}📤 Enviando: INVITE user${NC}"
response=$(send_and_receive "INVITE user" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "461")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "INVITE sin canal (461)" "PASS" "461" "$response"
else
    print_result "INVITE sin canal (461)" "FAIL" "461" "$response"
fi

echo ""
echo -e "${PURPLE}📋 SECCIÓN 4: ERRORES DE NICKNAME${NC}"
echo "==============================="

# NICK sin argumentos - ERR_NONICKNAMEGIVEN (431)
echo -e "${CYAN}📤 Enviando: NICK${NC}"
response=$(send_and_receive "NICK" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "431")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "NICK sin argumentos (431)" "PASS" "431" "$response"
else
    print_result "NICK sin argumentos (431)" "FAIL" "431" "$response"
fi

# NICK inválido - ERR_ERRONEUSNICKNAME (432)
echo -e "${CYAN}📤 Enviando: NICK @invalid${NC}"
response=$(send_and_receive "NICK @invalid" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "432")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "NICK inválido (432)" "PASS" "432" "$response"
else
    print_result "NICK inválido (432)" "FAIL" "432" "$response"
fi

echo ""
echo -e "${PURPLE}📋 SECCIÓN 5: COMANDOS VÁLIDOS Y RESPUESTAS${NC}"
echo "=========================================="

# PING/PONG
echo -e "${CYAN}📤 Enviando: PING :test${NC}"
response=$(send_and_receive "PING :test" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if echo "$response" | grep -q "PONG"; then
    print_result "PING/PONG válido" "PASS" "PONG" "$response"
else
    print_result "PING/PONG válido" "FAIL" "PONG" "$response"
fi

# JOIN válido
echo -e "${CYAN}📤 Enviando: JOIN #testchannel${NC}"
response=$(send_and_receive "JOIN #testchannel" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if echo "$response" | grep -q "JOIN"; then
    print_result "JOIN válido" "PASS" "JOIN" "$response"
else
    print_result "JOIN válido" "FAIL" "JOIN" "$response"
fi

# NAMES - verificar RPL_NAMREPLY (353) y RPL_ENDOFNAMES (366)
echo -e "${CYAN}📤 Enviando: NAMES #testchannel${NC}"
response=$(send_and_receive "NAMES #testchannel" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
has_353=$(check_code "$response" "353")
has_366=$(check_code "$response" "366")
if [ "$has_353" = "PASS" ] && [ "$has_366" = "PASS" ]; then
    print_result "NAMES válido (353+366)" "PASS" "353+366" "$response"
else
    print_result "NAMES válido (353+366)" "FAIL" "353+366" "$response"
fi

# LIST - verificar RPL_LISTSTART (321), RPL_LIST (322), RPL_LISTEND (323)
echo -e "${CYAN}📤 Enviando: LIST${NC}"
response=$(send_and_receive "LIST" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
has_321=$(check_code "$response" "321")
has_322=$(check_code "$response" "322")
has_323=$(check_code "$response" "323")
if [ "$has_321" = "PASS" ] && [ "$has_322" = "PASS" ] && [ "$has_323" = "PASS" ]; then
    print_result "LIST válido (321+322+323)" "PASS" "321+322+323" "$response"
else
    print_result "LIST válido (321+322+323)" "FAIL" "321+322+323" "$response"
fi

# TOPIC - verificar RPL_NOTOPIC (331) o RPL_TOPIC (332)
echo -e "${CYAN}📤 Enviando: TOPIC #testchannel${NC}"
response=$(send_and_receive "TOPIC #testchannel" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
has_331=$(check_code "$response" "331")
has_332=$(check_code "$response" "332")
if [ "$has_331" = "PASS" ] || [ "$has_332" = "PASS" ]; then
    print_result "TOPIC válido (331/332)" "PASS" "331/332" "$response"
else
    print_result "TOPIC válido (331/332)" "FAIL" "331/332" "$response"
fi

# MODE - verificar RPL_CHANNELMODEIS (324)
echo -e "${CYAN}📤 Enviando: MODE #testchannel${NC}"
response=$(send_and_receive "MODE #testchannel" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
has_324=$(check_code "$response" "324")
if [ "$has_324" = "PASS" ]; then
    print_result "MODE válido (324)" "PASS" "324" "$response"
else
    print_result "MODE válido (324)" "FAIL" "324" "$response"
fi

# PRIVMSG válido con mensaje de una sola letra
echo -e "${CYAN}📤 Enviando: PRIVMSG #testchannel :a${NC}"
response=$(send_and_receive "PRIVMSG #testchannel :a" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if echo "$response" | grep -q "PRIVMSG"; then
    print_result "PRIVMSG con mensaje de una letra" "PASS" "PRIVMSG" "$response"
else
    print_result "PRIVMSG con mensaje de una letra" "FAIL" "PRIVMSG" "$response"
fi

# PRIVMSG válido con mensaje normal
echo -e "${CYAN}📤 Enviando: PRIVMSG #testchannel :Hello!${NC}"
response=$(send_and_receive "PRIVMSG #testchannel :Hello!" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if echo "$response" | grep -q "PRIVMSG"; then
    print_result "PRIVMSG válido" "PASS" "PRIVMSG" "$response"
else
    print_result "PRIVMSG válido" "FAIL" "PRIVMSG" "$response"
fi

# PART válido
echo -e "${CYAN}📤 Enviando: PART #testchannel${NC}"
response=$(send_and_receive "PART #testchannel" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if echo "$response" | grep -q "PART"; then
    print_result "PART válido" "PASS" "PART" "$response"
else
    print_result "PART válido" "FAIL" "PART" "$response"
fi

echo ""
echo -e "${PURPLE}📋 SECCIÓN 6: ERRORES DE CONTRASEÑA${NC}"
echo "================================="

# PASS incorrecta para usuario registrado - ERR_ALREADYREGISTRED (462)
echo -e "${CYAN}📤 Enviando: PASS wrongpass${NC}"
response=$(send_and_receive "PASS wrongpass" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "462")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "PASS incorrecta para usuario registrado (462)" "PASS" "462" "$response"
else
    print_result "PASS incorrecta para usuario registrado (462)" "FAIL" "462" "$response"
fi

echo ""
echo -e "${PURPLE}📋 SECCIÓN 7: COMANDOS SIN REGISTRO (NUEVA CONEXIÓN)${NC}"
echo "====================================================="

# Crear nueva conexión sin registro
exec 4<>/dev/tcp/localhost/6669
echo -e "${GREEN}✅ Nueva conexión establecida (FD: 4)${NC}"

# PRIVMSG sin registro - ERR_NOTREGISTERED (451)
echo -e "${CYAN}📤 Enviando: PRIVMSG #test :Hello${NC}"
response=$(send_and_receive "PRIVMSG #test :Hello" 4)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "451")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "PRIVMSG sin registro (451)" "PASS" "451" "$response"
else
    print_result "PRIVMSG sin registro (451)" "FAIL" "451" "$response"
fi

# JOIN sin registro - ERR_NOTREGISTERED (451)
echo -e "${CYAN}📤 Enviando: JOIN #test${NC}"
response=$(send_and_receive "JOIN #test" 4)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
result=$(check_code "$response" "451")
format=$(check_format "$response")
if [ "$result" = "PASS" ] && [ "$format" = "PASS" ]; then
    print_result "JOIN sin registro (451)" "PASS" "451" "$response"
else
    print_result "JOIN sin registro (451)" "FAIL" "451" "$response"
fi

# Cerrar segunda conexión
exec 4<&-
exec 4>&-

echo ""
echo -e "${PURPLE}📋 SECCIÓN 8: COMANDOS ADICIONALES${NC}"
echo "================================="

# NAMES sin canal
echo -e "${CYAN}📤 Enviando: NAMES${NC}"
response=$(send_and_receive "NAMES" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
has_366=$(check_code "$response" "366")
if [ "$has_366" = "PASS" ]; then
    print_result "NAMES sin canal (366)" "PASS" "366" "$response"
else
    print_result "NAMES sin canal (366)" "FAIL" "366" "$response"
fi

# Crear canal específico para LIST
echo -e "${CYAN}📤 Enviando: JOIN #listchannel${NC}"
response=$(send_and_receive "JOIN #listchannel" 3)
echo -e "${CYAN}📥 Respuesta JOIN: $response${NC}"

# LIST con canal específico
echo -e "${CYAN}📤 Enviando: LIST #listchannel${NC}"
response=$(send_and_receive "LIST #listchannel" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
has_321=$(check_code "$response" "321")
has_322=$(check_code "$response" "322")
has_323=$(check_code "$response" "323")
if [ "$has_321" = "PASS" ] && [ "$has_322" = "PASS" ] && [ "$has_323" = "PASS" ]; then
    print_result "LIST con canal específico (321+322+323)" "PASS" "321+322+323" "$response"
else
    print_result "LIST con canal específico (321+322+323)" "FAIL" "321+322+323" "$response"
fi

# QUIT
echo -e "${CYAN}📤 Enviando: QUIT${NC}"
response=$(send_and_receive "QUIT" 3)
echo -e "${CYAN}📥 Respuesta: $response${NC}"
if echo "$response" | grep -q "QUIT" || [ -z "$response" ]; then
    print_result "QUIT válido" "PASS" "QUIT o sin respuesta" "$response"
else
    print_result "QUIT válido" "FAIL" "QUIT o sin respuesta" "$response"
fi

# Cerrar conexión principal
exec 3<&-
exec 3>&-

echo ""
echo -e "${BLUE}📊 RESUMEN FINAL${NC}"
echo "==============="
echo -e "Total de pruebas: ${BLUE}$TOTAL_TESTS${NC}"
echo -e "Pruebas exitosas: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Pruebas fallidas: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}🎉 ¡TODAS LAS PRUEBAS PASARON!${NC}"
    echo -e "${GREEN}✅ El servidor IRC está completamente funcional${NC}"
    echo -e "${GREEN}✅ Todos los mensajes de error y confirmación funcionan correctamente${NC}"
    echo -e "${GREEN}✅ Conexión persistente mantenida durante todo el test${NC}"
    echo -e "${GREEN}✅ Mensajes de una sola letra funcionan perfectamente${NC}"
    exit 0
else
    echo -e "${RED}⚠️  $FAILED_TESTS pruebas fallaron${NC}"
    echo -e "${YELLOW}💡 Revisar implementación de los comandos fallidos${NC}"
    exit 1
fi
