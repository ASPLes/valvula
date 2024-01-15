#!/bin/bash

gitlog-to-changelog  | sed  's/valvula: *//g' > ChangeLog
