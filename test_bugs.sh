#!/bin/bash

# Test script para encontrar bugs en el servidor IRC
PORT=1501
PASS=h

echo "=== TEST 1: Intentar JOIN sin estar registrado ==="
(
    sleep 1
    echo "JOIN #test"
    sleep 1
) | nc localhost $PORT &

sleep 3
pkill -P $$ nc

echo ""
echo "=== TEST 2: Intentar JOIN con nombre de canal inválido ==="
(
    sleep 1
    echo "PASS $PASS"
    echo "NICK testuser1"
    echo "USER test 0 * :Test User"
    sleep 1
    echo "JOIN test"  # Sin #
    echo "JOIN ##test"  # Doble ##
    echo "JOIN #"  # Solo #
    sleep 1
) | nc localhost $PORT &

sleep 4
pkill -P $$ nc

echo ""
echo "=== TEST 3: Intentar enviar PRIVMSG a canal donde no estás ==="
(
    sleep 1
    echo "PASS $PASS"
    echo "NICK testuser2"
    echo "USER test 0 * :Test User"
    sleep 1
    echo "PRIVMSG #test :hola"
    sleep 1
) | nc localhost $PORT &

sleep 4
pkill -P $$ nc

echo ""
echo "=== TEST 4: Intentar hacer JOIN dos veces al mismo canal ==="
(
    sleep 1
    echo "PASS $PASS"
    echo "NICK testuser3"
    echo "USER test 0 * :Test User"
    sleep 1
    echo "JOIN #test"
    sleep 1
    echo "JOIN #test"  # Segunda vez
    sleep 1
) | nc localhost $PORT &

sleep 5
pkill -P $$ nc

echo ""
echo "=== TEST 5: Enviar comandos con parámetros vacíos ==="
(
    sleep 1
    echo "PASS $PASS"
    echo "NICK testuser4"
    echo "USER test 0 * :Test User"
    sleep 1
    echo "JOIN"  # Sin parámetros
    echo "PRIVMSG"  # Sin parámetros
    echo "PART"  # Sin parámetros
    sleep 1
) | nc localhost $PORT &

sleep 5
pkill -P $$ nc

echo ""
echo "Tests completados. Revisa la salida del servidor para errores."

