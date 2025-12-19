(function () {
	const svg = document.querySelector("[data-map]");
	const pickupTable = document.querySelector("[data-pickup-table]");
	if (!svg || !pickupTable) return;

	// Same as in python
	// 0 = Empty
	// 1 = Straigt ahead
	// 2 = Crossing
	const mapGrid = generateGrid(6, 6);

	const mapData = {
		cellSize: 48,
		grid: mapGrid,
		gridRows: 6,
		gridCols: 6,
		pickups: [
			// Row 1 pickups (top row of grid)
			{ id: "P1", row: 1, col: 2, side: "right", hasCargo: false },
			{ id: "P2", row: 1, col: 4, side: "left", hasCargo: false },
			{ id: "P3", row: 1, col: 6, side: "right", hasCargo: false },
			{ id: "P4", row: 1, col: 8, side: "left", hasCargo: false },
			{ id: "P5", row: 1, col: 10, side: "right", hasCargo: false },

			// Row 3 pickups
			{ id: "P6", row: 3, col: 2, side: "right", hasCargo: false },
			{ id: "P7", row: 3, col: 4, side: "left", hasCargo: false },
			{ id: "P8", row: 3, col: 6, side: "left", hasCargo: false },
			{ id: "P9", row: 3, col: 8, side: "left", hasCargo: false },
			{ id: "P10", row: 3, col: 10, side: "right", hasCargo: false },

			// Row 5 pickups
			{ id: "P11", row: 5, col: 2, side: "right", hasCargo: false },
			{ id: "P12", row: 5, col: 4, side: "right", hasCargo: false },
			{ id: "P13", row: 5, col: 6, side: "right", hasCargo: false },
			{ id: "P14", row: 5, col: 8, side: "right", hasCargo: false },
			{ id: "P15", row: 5, col: 10, side: "left", hasCargo: false },

			// Row 7 pickups
			{ id: "P16", row: 7, col: 2, side: "left", hasCargo: false },
			{ id: "P17", row: 7, col: 4, side: "left", hasCargo: false },
			{ id: "P18", row: 7, col: 6, side: "left", hasCargo: false },
			{ id: "P19", row: 7, col: 8, side: "right", hasCargo: false },
			{ id: "P20", row: 7, col: 10, side: "left", hasCargo: false },

			// Row 9 pickups
			{ id: "P21", row: 9, col: 2, side: "left", hasCargo: false },
			{ id: "P22", row: 9, col: 4, side: "left", hasCargo: false },
			{ id: "P23", row: 9, col: 6, side: "right", hasCargo: false },
			{ id: "P24", row: 9, col: 8, side: "left", hasCargo: false },
			{ id: "P25", row: 9, col: 10, side: "left", hasCargo: false },

			//Row 11 pickups
			{ id: "P31", row: 11, col: 2, side: "right", hasCargo: false },
			{ id: "P32", row: 11, col: 4, side: "right", hasCargo: false },
			{ id: "P33", row: 11, col: 6, side: "left", hasCargo: false },
			{ id: "P34", row: 11, col: 8, side: "left", hasCargo: false },
			{ id: "P35", row: 11, col: 10, side: "right", hasCargo: false },

			// Column 1 pickups (rightmost column of grid)
			{ id: "P26", row: 2, col: 1, side: "left", hasCargo: false },
			{ id: "P27", row: 4, col: 1, side: "left", hasCargo: false },
			{ id: "P28", row: 6, col: 1, side: "left", hasCargo: false },
			{ id: "P29", row: 8, col: 1, side: "left", hasCargo: false },
			{ id: "P30", row: 10, col: 1, side: "right", hasCargo: false },

			// Column 3 pickups
			{ id: "P36", row: 2, col: 3, side: "left", hasCargo: false },
			{ id: "P37", row: 4, col: 3, side: "left", hasCargo: false },
			{ id: "P38", row: 6, col: 3, side: "right", hasCargo: false },
			{ id: "P39", row: 8, col: 3, side: "right", hasCargo: false },
			{ id: "P40", row: 10, col: 3, side: "right", hasCargo: false },

			// Column 5 pickups
			{ id: "P41", row: 2, col: 5, side: "right", hasCargo: false },
			{ id: "P42", row: 4, col: 5, side: "left", hasCargo: false },
			{ id: "P43", row: 6, col: 5, side: "left", hasCargo: false },
			{ id: "P44", row: 8, col: 5, side: "right", hasCargo: false },
			{ id: "P45", row: 10, col: 5, side: "right", hasCargo: false },

			// Column 7 pickups
			{ id: "P46", row: 2, col: 7, side: "right", hasCargo: false },
			{ id: "P47", row: 4, col: 7, side: "right", hasCargo: false },
			{ id: "P48", row: 6, col: 7, side: "left", hasCargo: false },
			{ id: "P49", row: 8, col: 7, side: "right", hasCargo: false },
			{ id: "P50", row: 10, col: 7, side: "right", hasCargo: false },

			// Column 9 pickups
			{ id: "P51", row: 2, col: 9, side: "left", hasCargo: false },
			{ id: "P52", row: 4, col: 9, side: "left", hasCargo: false },
			{ id: "P53", row: 6, col: 9, side: "right", hasCargo: false },
			{ id: "P54", row: 8, col: 9, side: "left", hasCargo: false },
			{ id: "P55", row: 10, col: 9, side: "right", hasCargo: false },

			// Column 11 pickups
			{ id: "P56", row: 2, col: 11, side: "left", hasCargo: false },
			{ id: "P57", row: 4, col: 11, side: "right", hasCargo: false },
			{ id: "P58", row: 6, col: 11, side: "left", hasCargo: false },
			{ id: "P59", row: 8, col: 11, side: "right", hasCargo: false },
			{ id: "P60", row: 10, col: 11, side: "left", hasCargo: false },
		],
	};

	const DIRECTION_VECTORS = {
		up: { dx: 0, dy: -1 },
		down: { dx: 0, dy: 1 },
		left: { dx: -1, dy: 0 },
		right: { dx: 1, dy: 0 },
	};

	function createSvgEl(name, attrs) {
		const el = document.createElementNS("http://www.w3.org/2000/svg", name);
		Object.entries(attrs || {}).forEach(([key, value]) =>
			el.setAttribute(key, value),
		);
		return el;
	}

	function cellCenter(row, col, size) {
		return {
			x: col * size + size / 2,
			y: row * size + size / 2,
		};
	}

	function isRoad(grid, row, col) {
		return grid[row]?.[col] > 0;
	}

	function determineAxis(pickup, grid) {
		if (pickup.axis) return pickup.axis;
		const hasHorizontal =
			isRoad(grid, pickup.row, pickup.col - 1) ||
			isRoad(grid, pickup.row, pickup.col + 1);
		const hasVertical =
			isRoad(grid, pickup.row - 1, pickup.col) ||
			isRoad(grid, pickup.row + 1, pickup.col);
		if (hasHorizontal && !hasVertical) return "horizontal";
		if (hasVertical && !hasHorizontal) return "vertical";
		return "horizontal";
	}

	function renderRoadNetwork(svgEl, grid, cellSize, gridRows, gridCols) {
		const rows = grid.length;
		const cols = grid[0]?.length ?? 0;
		const roadGroup = createSvgEl("g", {
			stroke: "var(--map-road-line)",
			"stroke-width": cellSize * 0.2,
			"stroke-linecap": "round",
			fill: "none",
		});

		for (let row = 0; row < rows; row += 1) {
			for (let col = 0; col < cols; col += 1) {
				if (!isRoad(grid, row, col)) continue;
				const start = cellCenter(row, col, cellSize);

				if (isRoad(grid, row, col + 1)) {
					const right = cellCenter(row, col + 1, cellSize);
					roadGroup.appendChild(
						createSvgEl("line", {
							x1: start.x,
							y1: start.y,
							x2: right.x,
							y2: right.y,
						}),
					);
				}

				if (isRoad(grid, row + 1, col)) {
					const down = cellCenter(row + 1, col, cellSize);
					roadGroup.appendChild(
						createSvgEl("line", {
							x1: start.x,
							y1: start.y,
							x2: down.x,
							y2: down.y,
						}),
					);
				}
			}
		}

		svgEl.appendChild(roadGroup);

		const nodeGroup = createSvgEl("g", {
			fill: "var(--map-cross-node)",
			stroke: "var(--map-cross-outline)",
			"stroke-width": 2,
		});
		const nodeRadius = cellSize * 0.18;

		for (let row = 0; row < rows; row += 1) {
			for (let col = 0; col < cols; col += 1) {
				if (grid[row]?.[col] !== 2) continue;
				const { x, y } = cellCenter(row, col, cellSize);
				nodeGroup.appendChild(
					createSvgEl("circle", {
						cx: x,
						cy: y,
						r: nodeRadius,
					}),
				);
			}
		}

		svgEl.appendChild(nodeGroup);
	}

	function renderPickups(svgEl, pickups, grid, cellSize) {
		const radius = cellSize * 0.17;
		const offset = cellSize * 0.35;
		pickups.forEach((pickup) => {
			const tileValue = grid[pickup.row]?.[pickup.col];
			if (!tileValue) return;
			const { x, y } = cellCenter(pickup.row, pickup.col, cellSize);
			const axis = determineAxis(pickup, grid);
			const side = pickup.side === "right" ? "right" : "left";
			let padX = x;
			let padY = y;

			if (axis === "horizontal") {
				padY += side === "left" ? -offset : offset;
			} else {
				padX += side === "left" ? -offset : offset;
			}

			svgEl.appendChild(
				createSvgEl("line", {
					x1: x,
					y1: y,
					x2: padX,
					y2: padY,
					stroke: "var(--map-road-line)",
					"stroke-width": 2,
				}),
			);

			const circle = createSvgEl("circle", {
				cx: padX,
				cy: padY,
				r: radius,
				fill: pickup.hasCargo
					? "var(--map-pad-ready)"
					: "var(--map-pad-empty)",
				stroke: "var(--map-pad-outline)",
				"stroke-width": 1.6,
				"data-pickup-id": pickup.id,
				style: "cursor: pointer; transition: all 0.2s ease;",
			});

			// Add hover listeners for highlighting table row
			circle.addEventListener("mouseenter", () =>
				highlightTableRow(pickup.id),
			);
			circle.addEventListener("mouseleave", () =>
				unhighlightTableRow(pickup.id),
			);

			const label = createSvgEl("text", {
				x: padX,
				y: padY - radius - 6,
				"text-anchor": "middle",
				"font-size": "11",
				"font-weight": "600",
				fill: "#e2e8f0",
			});
			label.textContent = pickup.id;

			svgEl.appendChild(circle);
			svgEl.appendChild(label);
		});
	}

	function renderStartPad(svgEl, cellSize) {
		const vector = DIRECTION_VECTORS.down;
		const { x, y } = cellCenter(mapData.gridRows * 2 - 1, mapData.gridCols * 2 - 1, cellSize);
		const distance = cellSize * 2.2;
		const endX = x + vector.dx * distance;
		const endY = y + vector.dy * distance;
		const circleRadius = cellSize * 0.4;

		svgEl.appendChild(
			createSvgEl("line", {
				x1: x,
				y1: y,
				x2: endX,
				y2: endY,
				stroke: "var(--map-depot)",
				"stroke-width": 3,
				"stroke-dasharray": "10 6",
			}),
		);

		svgEl.appendChild(
			createSvgEl("circle", {
				cx: endX,
				cy: endY,
				r: circleRadius,
				fill: "var(--map-depot)",
				stroke: "#0369a1",
				"stroke-width": 2,
			}),
		);
	}

	function formatCoordinate(col, row) {
		return `(${col}, ${row})`;
	}

	function capitalize(str) {
		if (!str) return "—";
		return str.charAt(0).toUpperCase() + str.slice(1);
	}

	function populatePickupTable(table, pickups, grid) {
		const tbody = table.querySelector("tbody");
		tbody.innerHTML = "";

		pickups.forEach((pickup) => {
			const tileValue = grid[pickup.row]?.[pickup.col];
			if (!tileValue) return;

			const rowEl = document.createElement("tr");
			rowEl.dataset.pickupId = pickup.id;

			const cells = [
				pickup.id,
				formatCoordinate(pickup.col, pickup.row),
				capitalize(pickup.side),
				pickup.hasCargo ? "Cargo waiting" : "Empty",
			];

			cells.forEach((value, index) => {
				const cell = document.createElement("td");
				if (index === cells.length - 1) {
					const badge = document.createElement("span");
					badge.className = "badge";
					badge.dataset.state = pickup.hasCargo ? "ready" : "empty";
					badge.textContent = value;
					cell.appendChild(badge);
				} else {
					cell.textContent = value;
				}
				rowEl.appendChild(cell);
			});

			// Add action button cell
			const actionCell = document.createElement("td");
			const actionBtn = document.createElement("button");
			actionBtn.className = pickup.hasCargo
				? "pickup-action-btn pickup-action-btn--remove"
				: "pickup-action-btn pickup-action-btn--add";
			actionBtn.textContent = pickup.hasCargo ? "Remove" : "Add";
			actionBtn.addEventListener("click", () =>
				togglePickupCargo(pickup),
			);
			actionCell.appendChild(actionBtn);
			rowEl.appendChild(actionCell);

			tbody.appendChild(rowEl);
		});
	}

	function computeViewBox(grid, cellSize) {
		const rows = grid.length;
		const cols = grid[0]?.length ?? 0;
		const baseWidth = cols * cellSize;
		const baseHeight = rows * cellSize + 2.2*cellSize;

		// Start with the grid bounds plus equal margins on all sides
		const margin = cellSize * 0.4;
		let minX = -margin;
		let minY = -margin;
		let maxX = baseWidth + margin;
		let maxY = baseHeight + margin;

		return {
			x: minX,
			y: minY,
			width: maxX - minX,
			height: maxY - minY,
		};
	}

	function highlightTableRow(pickupId) {
		const row = pickupTable.querySelector(
			`tr[data-pickup-id="${pickupId}"]`,
		);
		if (!row) return;

		row.classList.add("highlight-row");

		// Scroll to the row
		const container = pickupTable.closest(".pickup-table-container");
		if (container) {
			const rowTop = row.offsetTop;
			const rowHeight = row.offsetHeight;
			const containerHeight = container.clientHeight;
			const scrollTop = container.scrollTop;

			// Check if row is not fully visible
			if (
				rowTop < scrollTop ||
				rowTop + rowHeight > scrollTop + containerHeight
			) {
				row.scrollIntoView({ behavior: "smooth", block: "center" });
			}
		}
	}

	function unhighlightTableRow(pickupId) {
		const row = pickupTable.querySelector(
			`tr[data-pickup-id="${pickupId}"]`,
		);
		if (row) {
			row.classList.remove("highlight-row");
		}
	}

	async function togglePickupCargo(pickup) {
		const API_BASE = "http://10.42.0.1:5001";
		const endpoint = "/setupCargo";

		// Toggle the cargo state
		pickup.hasCargo = !pickup.hasCargo;

		// Send to backend
		try {
			console.log(
				`Toggling cargo for ${pickup.id} at (${pickup.col}, ${pickup.row}) to ${pickup.hasCargo}`,
			);
			await fetch(`${API_BASE}${endpoint}`, {
				method: "POST",
				headers: { "Content-Type": "application/json" },
				body: JSON.stringify({
					x: pickup.col,
					y: pickup.row,
					hasCargo: pickup.hasCargo,
				}),
			});
			console.log(`Cargo status updated for ${pickup.id}`);
		} catch (err) {
			console.warn(`Failed to update cargo status for ${pickup.id}`, err);
			// Retry without CORS
			try {
				await fetch(`${API_BASE}${endpoint}`, {
					method: "POST",
					headers: { "Content-Type": "application/json" },
					body: JSON.stringify({
						id: pickup.id,
						x: pickup.col,
						y: pickup.row,
						hasCargo: pickup.hasCargo,
					}),
					mode: "no-cors",
				});
				console.log(
					`Cargo status updated without CORS for ${pickup.id}`,
				);
			} catch (secondaryError) {
				console.error(
					`Failed to update cargo status for ${pickup.id}`,
					secondaryError,
				);
			}
		}

		// Re-render the map and table
		render();
	}

	function generateGrid(cols, rows) {
		const grid = [];

		for (let row = 0; row < 2*rows+1; row++) {
			grid[row] = [];
			for (let col = 0; col < 2*cols+1; col++) {
				let value = 0;
				
				if (row % 2 === 1) {
					value += 1;
				}
				
				if (col % 2 === 1) {
					value += 1;
				}

				if (col === 0 || col === 2*cols || row === 0 || row === 2*rows) {
					value = 0;
				}
				
				grid[row][col] = value;
			}
		}

		return grid;
	}

	function resizeGrid(newRows, newCols) {

		// Update the grid data
		mapData.grid = generateGrid(newCols, newRows);
		mapData.gridRows = newRows;
		mapData.gridCols = newCols;

		// Update display values
		document.querySelector('[data-map-rows]').textContent = newRows;
		document.querySelector('[data-map-cols]').textContent = newCols;

		// Re-render the map
		render();
	}

	function initMapControls() {
		// Add event listeners for map size controls
		document.querySelectorAll('[data-map-action]').forEach(button => {
			button.addEventListener('click', (e) => {
				const action = e.target.dataset.mapAction;
				const currentRows = mapData.gridRows;
				const currentCols = mapData.gridCols;

				switch (action) {
					case 'increase-rows':
						resizeGrid(currentRows + 1, currentCols);
						break;
					case 'decrease-rows':
						if (currentRows > 2) { // Minimum rows limit
							resizeGrid(currentRows - 1, currentCols);
						}
						break;
					case 'increase-cols':
						resizeGrid(currentRows, currentCols + 1);
						break;
					case 'decrease-cols':
						if (currentCols > 2) { // Minimum columns limit
							resizeGrid(currentRows, currentCols - 1);
						}
						break;
				}
			});
		});
	}

	function render() {
		const { grid, cellSize, pickups, gridRows, gridCols } = mapData;
		const viewBox = computeViewBox(grid, cellSize);

		svg.setAttribute(
			"viewBox",
			`${viewBox.x} ${viewBox.y} ${viewBox.width} ${viewBox.height}`,
		);
		svg.innerHTML = "";

		renderStartPad(svg, cellSize);
		renderRoadNetwork(svg, grid, cellSize, gridRows, gridCols);
		renderPickups(svg, pickups, grid, cellSize);
		populatePickupTable(pickupTable, pickups, grid);
	}

	// Initialize map controls and set initial display values
	initMapControls();
	
	// Set initial display values for grid dimensions
	document.querySelector('[data-map-rows]').textContent = mapData.gridRows;
	document.querySelector('[data-map-cols]').textContent = mapData.gridCols;

	window.warehouseMapData = mapData;
	render();
})();
