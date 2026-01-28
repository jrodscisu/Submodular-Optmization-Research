import matplotlib.pyplot as plt

# 1. Enter Your Data Coordinates Here
# Ensure all lists have the same number of items
k_values = [20, 30, 40, 50, 60]  # The X-axis (K)

# The Y-axis values for the first line
performance_greedy = [0.66, 0.65, 0.64, 0.64, 0.64]
performance_mod_greedy = [0.95, 0.97, 0.98, 0.98, 0.98]

# The Y-axis values for the second line

# 2. Create the Plot
plt.figure(figsize=(8, 6))

# Plot Line 1 (Method A)
plt.plot(k_values, performance_greedy, 
         marker='o', linestyle='-', color='red', label='Greedy', linewidth=2)

# Plot Line 2 (Method B)
plt.plot(k_values, performance_mod_greedy, 
         marker='s', linestyle='--', color='blue', label= 'Greedy with Predictions', linewidth=2)

# 3. Add Labels, Title, and Legend
plt.xlabel('K', fontsize=12)
plt.ylabel('Performance (Accuracy)', fontsize=12)
plt.title('Performance vs. K Parameter', fontsize=14)
plt.legend()
plt.grid(True, linestyle=':', alpha=0.6)

# 4. Save and Show
plt.tight_layout()
plt.ylim(0, 1.0)
plt.savefig('hints_all_optimal.png', dpi=300)
plt.show()

##################################################################################

k_values = [20, 30, 40, 50, 60]  # The X-axis (K)

# The Y-axis values for the first line
performance_greedy = [0.65, 0.64, 0.64, 0.64, 0.64]

# The Y-axis values for the second line
performance_mod_greedy = [0.77, 0.79, 0.78, 0.81, 0.80]

# 2. Create the Plot
plt.figure(figsize=(8, 6))

# Plot Line 1 (Method A)
plt.plot(k_values, performance_greedy, 
         marker='o', linestyle='-', color='red', label='Greedy', linewidth=2)

# Plot Line 2 (Method B)
plt.plot(k_values, performance_mod_greedy, 
         marker='s', linestyle='--', color='blue', label= 'Greedy with Predictions', linewidth=2)

# 3. Add Labels, Title, and Legend
plt.xlabel('K', fontsize=12)
plt.ylabel('Performance (Accuracy)', fontsize=12)
plt.title('Performance vs. K Parameter', fontsize=14)
plt.legend()
plt.grid(True, linestyle=':', alpha=0.6)

# 4. Save and Show
plt.tight_layout()
plt.ylim(0, 1.0)
plt.savefig('100_bad_predictions.png', dpi=300)
plt.show()



##################################################################################



per_values = [1/100, 2/100, 4/100, 8/100, 16/100, 32/100, 64/100, 100/100]  # The X-axis (K)

# The Y-axis values for the first line
performance_greedy = [0.66, 0.65, 0.65, 0.66, 0.65, 0.65, 0.65, 0.65]

# The Y-axis values for the second line
performance_mod_greedy = [0.90, 0.89, 0.89, 0.87, 0.87, 0.83, 0.76, 0.71]

# 2. Create the Plot
plt.figure(figsize=(8, 6))

# Plot Line 1 (Method A)
plt.plot(per_values, performance_greedy, 
         marker='o', linestyle='-', color='red', label='Greedy', linewidth=2)

# Plot Line 2 (Method B)
plt.plot(per_values, performance_mod_greedy, 
         marker='s', linestyle='--', color='blue', label= 'Greedy with Predictions', linewidth=2)

# 3. Add Labels, Title, and Legend
plt.xlabel('Pr. of getting a bad prediction', fontsize=12)
plt.ylabel('Performance (Accuracy)', fontsize=12)
plt.title('Performance vs. Pr. of getting a bad predictions', fontsize=14)
plt.legend()
plt.grid(True, linestyle=':', alpha=0.6)

# 4. Save and Show
plt.tight_layout()
plt.ylim(0, 1.0)
plt.savefig('mixed_predictions.png', dpi=300)
plt.show()