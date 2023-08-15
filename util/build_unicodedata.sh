#!/bin/bash
set -C -e -o pipefail
shopt -s lastpipe
D="$(dirname $0)"
[[ "$D" =~ ^/ ]] || D="$(pwd)/$D"

exec >| "$D/unicodedata.cpp"

empty="{}"
grep -v ';;;$' "$D/UnicodeData.txt" | \
while IFS=';' read -r code_value character_name general_category canonical_combining_classes bidirectional_category character_decomposition_mapping decimal_digit_value digit_value numeric_value mirrored unicode_1_name iso_10646_comment uppercase_mapping lowercase_mapping titlecase_mapping remainning ; do
  [ -z "$uppercase_mapping" ] && uppercase_mapping=$code_value
  [ -z "$lowercase_mapping" ] && lowercase_mapping=$code_value
  [ -z "$titlecase_mapping" ] && titlecase_mapping=$code_value
  case_mapping[0x$code_value]="  { 0x$code_value, 0x$uppercase_mapping, 0x$lowercase_mapping, 0x$titlecase_mapping }, // $character_name $general_category"
done

cat <<EOF # header
std::vector<Utf8String::UnicodeCaseMapping> Utf8String::_case_mapping = {
EOF

#echo $code_values
for code_value in ${!case_mapping[@]}; do
  echo "${case_mapping[$code_value]}"
done

cat <<EOF # footer
};
EOF

