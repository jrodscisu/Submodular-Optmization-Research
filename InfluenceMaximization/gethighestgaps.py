def reformat_movie_data(input_file, output_file):
    try:
        gaps = []
        with open(input_file, 'r') as infile:
            for line in infile:
                # Remove whitespace and split by the colon delimiter
                parts = line.strip().split(':')
                
                # Check if the line has the expected 2 parts
                if len(parts) == 2:
                    movie = parts[0]
                    gap = int(parts[1])
                    
                    # Store the pair (gap, movie)
                    gaps.append((gap, movie))
        
        # Sort by gap in descending order
        gaps.sort(key=lambda x: x[0], reverse=True)
        
        # Write to output file
        with open(output_file, 'w') as outfile:
            for gap, movie in gaps:
                outfile.write(f"{gap} {movie}\n")
        
        print(f"Successfully processed! Check '{output_file}' for the results.")
        
    except FileNotFoundError:
        print("Error: The input file was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Usage
reformat_movie_data('gaps.txt', 'sorted_gaps.txt')