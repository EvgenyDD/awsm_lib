.PHONY: tests
tests: $(EXECUTABLE)
	@echo "======================================================="
	@-sudo valgrind --tool=memcheck \
		--trace-children=yes \
		--demangle=yes \
		--log-file="${TEST_OUTPUT}.vg.out" \
		--leak-check=full \
		--show-reachable=yes \
		--run-libc-freeres=yes \
		-s \
		$< ${REDIR_OUTPUT}
	@if ! tail -1 "${TEST_OUTPUT}.vg.out" | grep -q "ERROR SUMMARY: 0 errors"; then \
		echo "==== ERROR: valgrind found errors during execution ===="; \
		tail -1 "${TEST_OUTPUT}.vg.out"; \
	else \
		echo "================ No errors found ======================"; \
	fi