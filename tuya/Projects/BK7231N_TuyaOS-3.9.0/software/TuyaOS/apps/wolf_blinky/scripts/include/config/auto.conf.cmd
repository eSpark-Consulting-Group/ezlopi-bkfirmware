deps_config := \
	ezlopi_user_config/Kconfig \
	Kconfig \

scripts/include/config/auto.conf: $(deps_config)


$(deps_config): ;
