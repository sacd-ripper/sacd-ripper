#TARGETS := libid3tag libcommon libunself libpatchutils libsacd
TARGETS := libsacd

all:
	@for TARGET in $(TARGETS); do $(MAKE) --no-print-directory -C $$TARGET; done

clean:
	@for TARGET in $(TARGETS); do $(MAKE) --no-print-directory -C $$TARGET clean; done
