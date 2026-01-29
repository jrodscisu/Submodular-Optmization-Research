import matplotlib.pyplot as plt

# 1. Enter Your Data Coordinates Here
# Ensure all lists have the same number of items
k_values = [20, 30, 40, 50, 60]  # The X-axis (K)

# The Y-axis values for the first line
performance_greedy = [0.661, 0.6496, 0.6449, 0.6424, 0.6406]
performance_mod_greedy = [0.800, 0.7929, 0.7921, 0.7903, 0.7888]

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
plt.ylabel('Approximation ratio', fontsize=12)
plt.title('Approximation ratio vs. K Parameter', fontsize=14)
plt.legend()
plt.grid(True, linestyle=':', alpha=0.6)

# 4. Save and Show
plt.tight_layout()
# plt.ylim(0, 1.0)
plt.savefig('hints_all_optimal.png', dpi=300)
plt.show()

##################################################################################

k_values = [20, 30, 40, 50, 60]  # The X-axis (K)

# The Y-axis values for the first line
performance_greedy = [0.6610, 0.6496, 0.6449, 0.6424, 0.6406]

# The Y-axis values for the second line
performance_mod_greedy = [0.6521, 0.6439, 0.6409, 0.6399, 0.6389]

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
plt.ylabel('Approximation ratio', fontsize=12)
plt.title('Approximation ratiovs. K Parameter', fontsize=14)
plt.legend()
plt.grid(True, linestyle=':', alpha=0.6)

# 4. Save and Show
plt.tight_layout()
# plt.ylim(0, 1.0)
plt.savefig('100_bad_predictions.png', dpi=300)
plt.show()



##################################################################################



per_values = [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]  # The X-axis (K)

# The Y-axis values for the first line
performance_greedy = [0.661012, 0.661012,0.661012,0.661012,0.661012,0.661012,0.661012,0.661012,0.661012,0.661012,0.661012]

# The Y-axis values for the second line
performance_mod_greedy = [0.7211, 0.7149, 0.7084, 0.7008, 0.6947, 0.6871, 0.6808, 0.6733, 0.6662, 0.6591, 0.6521]

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
plt.ylabel('Approximation ratio(Accuracy)', fontsize=12)
plt.title('Approximation ratio vs. Pr. of getting a bad predictions', fontsize=14)
plt.legend()
plt.grid(True, linestyle=':', alpha=0.6)

# 4. Save and Show
plt.tight_layout()
# plt.ylim(0, 1.0)
plt.savefig('mixed_predictions.png', dpi=300)
plt.show()