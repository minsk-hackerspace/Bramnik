#!/bin/bash

set -e

. /etc/bramnik/bramnik.env

cd `dirname $0`

code=`./env/bin/python ./bramnik_mgr.py code emit $DOD_CODE_ISSUER $DOD_CODE_TTL DoD | sed 's/^.*: \([0-9]\{4\}\) .*$/\1/'`

echo "DoD code: $code"

for chat_id in $TELEGRAM_CHATS; do
	curl -s -X POST \
		-H 'Content-Type: application/json' \
		-d "{\"chat_id\": \"$chat_id\", \"text\": \"Код для дня открытых дверей сегодня: $code\n#dodcode\", \"disable_notification\": true}" \
		https://api.telegram.org/bot$TELEGRAM_TOKEN/sendMessage > /dev/null
done
