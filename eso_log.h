#pragma once

#define ESO_VERBOSE_DEBUG

void log_printf(const char* format, ...);
void log_verbose(const char* format, ...);
void log_ingestion(char character);
void log_ingestion_marker(const char* format, ...);