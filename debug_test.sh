#!/bin/bash

echo "🔍 DEBUG: Test simple para ver qué está pasando"
echo "================================================"

# Crear conexión
exec 3<>/dev/tcp/localhost/6669

# Registro básico
echo -e "PASS testpass\r\n" >&3
sleep 0.1
echo -e "NICK testuser\r\n" >&3
sleep 0.1
echo -e "USER testuser 0 * :Test User\r\n" >&3
sleep 0.5

# Leer mensajes de bienvenida
while read -t 0.1 -r line <&3 2>/dev/null; do
    echo "Welcome: $line"
done

# JOIN a un canal
echo -e "JOIN #test\r\n" >&3
sleep 0.1

# Leer respuesta del JOIN
while read -t 0.1 -r line <&3 2>/dev/null; do
    echo "JOIN: $line"
done

# Probar mensaje de una sola letra con debug
echo "📤 Enviando: PRIVMSG #test :a"
echo -e "PRIVMSG #test :a\r\n" >&3
sleep 0.2

# Leer respuesta
echo "📥 Respuesta:"
while read -t 0.1 -r line <&3 2>/dev/null; do
    echo "$line"
done

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "✅ Debug test completado"
