#!/bin/bash

rm -f *.log

dabc_run ib-test.xml dellog

dabc_run ib-test.xml run

echo call dabc_run ib-test.xml getlog

