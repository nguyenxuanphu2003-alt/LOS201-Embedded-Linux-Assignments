set pagination off
set confirm off
set debuginfod enabled off
set logging file ../Task3/logs/gdb_sms_push_pop.txt
set logging overwrite on
set logging enabled on

break buffer_push
commands 1
silent
printf "\n===== SENSOR: BUFFER PUSH =====\n"
info threads
print q->head
print q->tail
print q->size
thread apply all bt 4
disable 1
enable 2
continue
end

break buffer_pop
commands 2
silent
printf "\n===== PROCESSOR: BUFFER POP =====\n"
info threads
print q->head
print q->tail
print q->size
thread apply all bt 4
disable 2
continue
end

disable 2
run
set logging enabled off
quit
