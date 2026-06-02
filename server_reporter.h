/*
 * Copyright (C) 2025 James L. Walker, Kiro (AI Assistant)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SERVER_REPORTER_H
#define SERVER_REPORTER_H

enum ReportResult {
    REPORT_SUCCESS,            // Server accepted the readings (inserted + skipped duplicates)
    REPORT_CONNECT_FAILED,     // Could not connect to server
    REPORT_TIMEOUT,            // Connected but response timed out
    REPORT_PARSE_ERROR         // Response received but could not be parsed
};

/**
 * Attempt to transmit all cached readings to the tempmon2 ingest endpoint.
 *
 * Reads all entries from the reading cache, serializes them to JSON,
 * and sends an HTTP POST to the configured ingest endpoint.
 *
 * On success, removalCount is set to the number of readings the caller
 * should remove from the cache (inserted_count + skipped_count from server).
 *
 * @param removalCount  Output: number of readings to remove from cache on success
 * @return ReportResult indicating the outcome of the transmission attempt
 *
 * Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6
 */
ReportResult serverReporter_send(int& removalCount);

#endif
