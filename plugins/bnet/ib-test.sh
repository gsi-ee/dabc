#!/bin/bash

rm *.log

dabc_run ib-test.xml dellog

dabc_run ib-test.xml run

# dabc_run ib-test.xml getlog

