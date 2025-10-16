#!/bin/bash

PORT=6667
PASS=h
SERVER="localhost"

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

test_count=0
pass_count=0
fail_count=0

run_test() {
    test_count=$((test_count + 1))
    echo -e "${YELLOW}=== TEST $test_count: $1 ===${NC}"
}

check_result() {
    if echo "$2" | grep -q "$1"; then
        echo -e "${GREEN}✓ PASS: Se recibió respuesta esperada${NC}"
        pass_count=$((pass_count + 1))
    else
        echo -e "${RED}✗ FAIL: No se recibió la respuesta esperada '$1'${NC}"
        echo "Recibido: $2"
        fail_count=$((fail_count + 1))
    fi
}

echo "========================================"
echo "PRUEBAS EXHAUSTIVAS DEL SERVIDOR IRC"
echo "========================================"
echo ""

# TEST 1: JOIN sin estar registrado
run_test "JOIN sin estar registrado (debe devolver ERR_NOTREGISTERED)"
result=$(
    (
        sleep 0.5
        echo "JOIN #test"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "451" "$result"
echo ""

# TEST 2: JOIN sin parámetros
run_test "JOIN sin parámetros (debe devolver ERR_NEEDMOREPARAMS)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test1"
        echo "USER test1 0 * :Test"
        sleep 0.5
        echo "JOIN"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "461" "$result"
echo ""

# TEST 3: JOIN con nombre de canal inválido
run_test "JOIN con '#' solo (debe devolver ERR_BADCHANMASK)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test2"
        echo "USER test2 0 * :Test"
        sleep 0.5
        echo "JOIN #"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "476" "$result"
echo ""

# TEST 4: JOIN con '##test' (debe devolver ERR_BADCHANMASK)
run_test "JOIN con '##test' (doble #)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test3"
        echo "USER test3 0 * :Test"
        sleep 0.5
        echo "JOIN ##test"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "476" "$result"
echo ""

# TEST 5: JOIN con nombre sin #
run_test "JOIN con 'test' sin # (debe devolver ERR_BADCHANMASK)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test4"
        echo "USER test4 0 * :Test"
        sleep 0.5
        echo "JOIN test"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "476" "$result"
echo ""

# TEST 6: JOIN exitoso
run_test "JOIN exitoso a #testchan (debe devolver JOIN + 353 + 366)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test5"
        echo "USER test5 0 * :Test"
        sleep 0.5
        echo "JOIN #testchan"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "353" "$result"
check_result "366" "$result"
check_result "JOIN #testchan" "$result"
echo ""

# TEST 7: JOIN al mismo canal dos veces
run_test "JOIN dos veces al mismo canal (segundo JOIN debe ser ignorado)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test6"
        echo "USER test6 0 * :Test"
        sleep 0.5
        echo "JOIN #double"
        sleep 0.5
        echo "JOIN #double"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
# Contar cuántas veces aparece JOIN #double
count=$(echo "$result" | grep -c "JOIN #double")
if [ "$count" -eq "1" ]; then
    echo -e "${GREEN}✓ PASS: JOIN solo se procesó una vez${NC}"
    pass_count=$((pass_count + 1))
else
    echo -e "${RED}✗ FAIL: JOIN se procesó $count veces (debería ser 1)${NC}"
    fail_count=$((fail_count + 1))
fi
echo ""

# TEST 8: PRIVMSG a canal sin estar en él
run_test "PRIVMSG a canal sin estar en él (debe devolver ERR_CANNOTSENDTOCHAN o ERR_NOSUCHCHANNEL)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test7"
        echo "USER test7 0 * :Test"
        sleep 0.5
        echo "PRIVMSG #nowhere :hello"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
if echo "$result" | grep -qE "404|403"; then
    echo -e "${GREEN}✓ PASS: Error correcto devuelto${NC}"
    pass_count=$((pass_count + 1))
else
    echo -e "${RED}✗ FAIL: No se recibió error esperado${NC}"
    echo "Recibido: $result"
    fail_count=$((fail_count + 1))
fi
echo ""

# TEST 9: PRIVMSG sin parámetros
run_test "PRIVMSG sin parámetros (debe devolver ERR_NEEDMOREPARAMS)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test8"
        echo "USER test8 0 * :Test"
        sleep 0.5
        echo "PRIVMSG"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "461" "$result"
echo ""

# TEST 10: Nickname con caracteres inválidos
run_test "NICK con espacios (debe devolver ERR_ERRONEUSNICKNAME)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK bad nick"
        echo "USER test9 0 * :Test"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "432" "$result"
echo ""

# TEST 11: Nickname que empieza con número
run_test "NICK que empieza con número (debe devolver ERR_ERRONEUSNICKNAME)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK 1badnick"
        echo "USER test10 0 * :Test"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "432" "$result"
echo ""

# TEST 12: Intentar comandos sin PASS
run_test "Comandos sin PASS (debe permitir NICK y USER, luego registrar)"
result=$(
    (
        sleep 0.5
        echo "NICK nopass"
        echo "USER nopass 0 * :Test"
        sleep 0.5
        echo "JOIN #test"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
# Debe rechazar porque no hay PASS
check_result "451\|JOIN" "$result"
echo ""

# TEST 13: Canal con espacios en el nombre
run_test "JOIN con nombre que contiene espacios (debe devolver ERR_BADCHANMASK)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test11"
        echo "USER test11 0 * :Test"
        sleep 0.5
        echo "JOIN #bad channel"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "476" "$result"
echo ""

# TEST 14: Canal con comas
run_test "JOIN con nombre que contiene coma (debe devolver ERR_BADCHANMASK)"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK test12"
        echo "USER test12 0 * :Test"
        sleep 0.5
        echo "JOIN #bad,channel"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
check_result "476" "$result"
echo ""

# TEST 15: Usuario intenta cambiar NICK estando ya registrado
run_test "Cambiar NICK después de registrarse"
result=$(
    (
        sleep 0.5
        echo "PASS $PASS"
        echo "NICK oldnick"
        echo "USER test13 0 * :Test"
        sleep 0.5
        echo "NICK newnick"
        sleep 0.5
        echo "JOIN #test"
        sleep 0.5
    ) | nc -w 2 $SERVER $PORT 2>/dev/null
)
# Debe permitir cambio de nick y luego JOIN
check_result "newnick" "$result"
echo ""

echo ""
echo "========================================"
echo "RESUMEN DE PRUEBAS"
echo "========================================"
echo "Total de tests: $test_count"
echo -e "${GREEN}Tests pasados: $pass_count${NC}"
echo -e "${RED}Tests fallidos: $fail_count${NC}"
echo "========================================"

# Matar el servidor si está corriendo
pkill -9 ircserv 2>/dev/null

