#!/bin/bash
exit "$(amixer get Master | grep '%' | tr -d '[]%' | awk '{print $5}' | head -n1)"
