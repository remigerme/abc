import re
import json
import os
import csv


def extract_info(file_path):
    """
    Extrait les nombres P, D, F, ainsi que les temps d'exécution de Sim et Sat
    à partir du contenu d'un fichier.
    """
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
    except FileNotFoundError:
        return {"error": f"Le fichier {file_path} n'a pas été trouvé."}

    # Regex pour trouver les lignes contenant P, D, F.
    # Cherche "P = <nombre> D = <nombre> F = <nombre>"
    pdf_regex = re.compile(r"P\s*=\s*(\d+)\s*D\s*=\s*(\d+)\s*F\s*=\s*(\d+)")

    # Regex pour trouver les temps d'exécution.
    # Cherche "Sim = <temps> sec" et "Sat = <temps> sec"
    time_regex = re.compile(r"(Sim|Sat)\s*=\s*([\d.]+)\s*sec")

    pdf_matches = pdf_regex.findall(content)
    time_matches = time_regex.findall(content)

    results = {"pdf_stats": [], "times": {}}

    for p, d, f in pdf_matches:
        results["pdf_stats"].append({"P": int(p), "D": int(d), "F": int(f)})

    for name, value in time_matches:
        results["times"][name] = float(value)

    return results


def process_directory(root_dir, csv_writer, write_individual_results=False):
    """
    Parcourt un répertoire, analyse les fichiers .out, écrit les résultats si demandé,
    et retourne les moyennes des ratios.
    """
    if not os.path.isdir(root_dir):
        print(f"Avertissement : Le répertoire {root_dir} n'existe pas.")
        return (0, 0)

    df_ratios = []
    time_ratios = []

    for subdir, _, files in os.walk(root_dir):
        for file in files:
            if file.endswith(".out"):
                file_path = os.path.join(subdir, file)
                data = extract_info(file_path)

                if "error" in data or not (data["pdf_stats"] or data["times"]):
                    continue

                circuit_name = os.path.splitext(file)[0]

                df_ratio = 0
                if data["pdf_stats"]:
                    stats = data["pdf_stats"][0]
                    p, d, f = stats["P"], stats["D"], stats["F"]
                    denominator = p + d + f
                    if denominator > 0:
                        df_ratio = (d + f) / denominator

                time_ratio = 0
                if "Sim" in data["times"] and "Sat" in data["times"]:
                    sim_time = data["times"]["Sim"]
                    sat_time = data["times"]["Sat"]
                    time_ratio = sim_time / (sat_time + sim_time)

                df_ratios.append(df_ratio)
                time_ratios.append(time_ratio)

                if write_individual_results:
                    csv_writer.writerow(
                        [circuit_name, round(df_ratio, 2), round(time_ratio, 2)]
                    )

    avg_df_ratio = sum(df_ratios) / len(df_ratios) if df_ratios else 0
    avg_time_ratio = sum(time_ratios) / len(time_ratios) if time_ratios else 0

    return (avg_df_ratio, avg_time_ratio)


def main():
    """
    Analyse les benchmarks, génère un fichier CSV avec les résultats et les moyennes
    pour chaque catégorie de benchmarks.
    """
    output_csv_file = "results.csv"

    epfl_dir = "benchmark/epfl_processed"
    beem_dir = "benchmark/beem_processed"
    cpu_dir = "benchmark/aig_cpu_processed"

    with open(output_csv_file, "w", newline="") as csvfile:
        csv_writer = csv.writer(csvfile)
        # csv_writer.writerow(["circuit_name", "DF_ratio", "time_ratio"])

        # Traiter les fichiers EPFL et écrire les résultats individuels
        avg_df_epfl, avg_time_epfl = process_directory(
            epfl_dir, csv_writer, write_individual_results=True
        )

        # Traiter les fichiers BEEM sans écrire les résultats individuels
        avg_df_beem, avg_time_beem = process_directory(
            beem_dir, csv_writer, write_individual_results=False
        )

        # _ = process_directory(cpu_dir, csv_writer, write_individual_results=True)

        # Écrire les moyennes
        csv_writer.writerow(
            ["average EPFL", round(avg_df_epfl, 2), round(avg_time_epfl, 2)]
        )
        csv_writer.writerow(
            ["average BEEM", round(avg_df_beem, 2), round(avg_time_beem, 2)]
        )

    print(f"Les résultats ont été exportés dans {output_csv_file}")


if __name__ == "__main__":
    main()
