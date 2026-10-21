# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(read-code-seg) begin
(read-code-seg) open "sample.txt"
read-code-seg: exit(-1)
EOF
pass;
