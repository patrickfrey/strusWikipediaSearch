#!/bin/sed -f

s/\x27\x27/\"/g
s/nbsp[\;]/ /g
s/\&[a-z]*\;/ /g
s/\xe2\x80\x93/ /g
s/\xe2\x80\x94/ /g
s/\xe2\x80\x98/ /g
s/\xe2\x80\x99/ /g
s/\xe2\x85\x9b/ /g
s/\xe2\x86\x92/ /g
s/\xe2\x86\x94/ /g
s/\xe2\x87\x86/ /g
s/\xe2\x88\x92/ /g
s/\xe2\x89\xa4/ /g
s/\xc2\xab/ /g
s/\xc2\xb1/ /g
s/\xc2\xb7/ /g
s/\xc2\xbb/ /g
s/\xc2\xbc/ /g
s/\xc2\xbd/ /g
s/\xc2\xbe/ /g
s/\xc3\x97/ /g

