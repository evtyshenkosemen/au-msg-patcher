#!/bin/bash
TESTS_DIR=libAuMsgParserTests
SRC_DIR=libAuMsgParser

#restore lib config
if ! [ -e ./$SRC_DIR/.env ]
then
    cp ./$SRC_DIR/default.env ./$SRC_DIR/.env
fi

#restore tests config
if ! [ -e ./$TESTS_DIR/.env ]
then
    cp ./$TESTS_DIR/default.env ./$TESTS_DIR/.env
fi
