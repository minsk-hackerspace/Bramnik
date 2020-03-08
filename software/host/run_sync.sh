#!/usr/bin/env bash

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
. /etc/bramnik/bramnik.env

tmpfile=$(mktemp /tmp/bramnik.XXXXXX)
curl https://hackerspace.by/bramnik -H "Accept: application/json" -H "Authorization: Bearer ${BRAMNIK_API_TOKEN}" >$tmpfile
cd $SCRIPTPATH && env/bin/python bramnik_mgr.py user sync $tmpfile
rm $tmpfile
