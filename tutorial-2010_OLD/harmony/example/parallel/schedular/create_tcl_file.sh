#!/bin/bash

## This is where you define the parameters
## For this example, it is only one parameter.


echo "  harmonyApp APPTest {
    { harmonyBundle param1 {
       int {0 19 1} global
    }
    }
    { obsGoodness -1000 1000 global }
    { predGoodness -300 -100 }
    } $1"
