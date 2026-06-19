"""Generate PDF_TEXT2.md embedding summaries from PDF_TEXT.md section keys."""
from __future__ import annotations

import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

SECTION_RE = re.compile(r"^## `(.+?)`\s*$", re.MULTILINE)

# Human-authored summaries for agent retrieval / embeddings (one paragraph each).
SUMMARIES: dict[str, str] = {
    "059UAP00011-1.png": (
        "Cover page of a October 30, 2001 CONFIDENTIAL State Department cable from U.S. Embassy "
        "Moscow (telegram 13169) titled “UFOs Over Georgia: Strange Encounters of an MFA Kind.” "
        "It is routing metadata for a diplomatic report on Caucasus security, Russian military "
        "activity in Abkhazia, and Georgian allegations of airspace violations—not a scientific UFO study."
    ),
    "059UAP00011-2.png": (
        "Body of the Moscow 13169 cable summarizing October 30 talks with Russian Deputy Foreign "
        "Minister Mamedov and MFA Georgia desk chief Tereoken. Russia denies bombing the Kodori Gorge; "
        "discusses Russian withdrawals from Gudauta, Abkhaz obstruction of observers, and U.S. concern "
        "that alleged Russian air incursions could harm the upcoming U.S.–Russia summit."
    ),
    "059UAP00011-3.png": (
        "Continuation of Moscow 13169: Tereoken suggests unidentified aircraft over Kodori might as well "
        "be “UFOs” because Moscow lacks technical means to verify foreign planes. Covers Russian train "
        "movements from Gudauta, refusal of outside observers, and Georgian parliamentary speaker Zvania’s "
        "Moscow visit."
    ),
    "059UAP00011-4.png": (
        "Second section (02 of 02) of Moscow cable 13169 on Georgia/Caucasus politics: Zvania–Seleznev "
        "discussions, dismissal of press rumors about Defense Minister Ivanov visiting Tbilisi, and "
        "broader U.S.–Russia relations framing around the “UFOs over Georgia” subject line."
    ),
    "059UAP00011-5.png": (
        "Closing administrative page of Moscow 13169 with dissemination and declassification markings "
        "(Released in Full, page 5 of 5). No substantive policy text beyond archive handling."
    ),
    "059UAP00012-1.png": (
        "November 12, 2004 SENSITIVE cable from U.S. Embassy Ashgabat on Turkmenistan civil society. "
        "Subject line references “UFOs” metaphorically: a Turkmen NGO branding itself around UFOlogy to "
        "register businesses, distribute aid, and navigate Niyazov-era restrictions—not an aerial phenomena report."
    ),
    "059UAP00012-2.png": (
        "Ashgabat cable describing the “UFOlogists of Turkmenabat” NGO as a practical civil-society partner "
        "for business registration and humanitarian work, using popular fascination with UFOs as social cover "
        "in authoritarian Turkmenistan."
    ),
    "059UAP00012-3.png": (
        "Further Ashgabat reporting on the Union of UFOlogists (UOU): membership scale, post-independence "
        "business-registration assistance, and how the group presents itself to local authorities."
    ),
    "059UAP00012-4.png": (
        "Ashgabat cable on UOU plans for an independent newsletter (999-copy limit to evade media law) and "
        "a USAID grant proposal for printing equipment—illustrating creative NGO tactics under Turkmen censorship."
    ),
    "059UAP00012-5.png": (
        "Final administrative page of the Ashgabat Turkmenistan/UFOlogists cable with classification and "
        "release markings only."
    ),
    "059UAP00013-1.png": (
        "September 16, 2023 UNCLASSIFIED Mexico City “Weekly Political Blotter” covering MORENA party "
        "infighting, INE election commissions, Mexico City security politics, and—later in the set—congressional "
        "UAP/alien-corpse hearings linked to Jaime Maussan."
    ),
    "059UAP00013-2.png": (
        "Mexico blotter section on Marcelo Ebrard challenging MORENA’s presidential selection process, "
        "alleging irregularities, and threatening to leave the party if his complaints are rejected."
    ),
    "059UAP00013-3.png": (
        "Mexico blotter on INE commission composition (anti-MORENA majorities on key bodies) and the "
        "resignation of Mexico City Security Secretary Omar García Harfuch amid speculation he will run "
        "for head of government."
    ),
    "059UAP00013-4.png": (
        "Mexico blotter: former MORENA Senate leader Ricardo Monreal declines a Mexico City head-of-government "
        "run; assesses the race as likely between García Harfuch and Iztapalapa mayor Clara Brugada."
    ),
    "059UAP00013-5.png": (
        "Mexico blotter security item: assassination of a newly appointed Guerrero state prosecutor in Tierra "
        "Caliente and related kidnapping of another prosecutor—organized-crime violence in southern Mexico."
    ),
    "059UAP00013-6.png": (
        "Mexico blotter on Jaime Maussan presenting alleged non-human remains to Congress; U.S. Navy pilot "
        "Ryan Graves criticizes the stunt; scientists previously debunked similar Maussan claims. Key UAP/politics crossover page."
    ),
    "059UAP00013-7.png": (
        "Distribution and release page for the Mexico City weekly blotter, listing Washington national-security "
        "recipients (NSC, CIA, DIA, NORTHCOM, etc.) and declassification footer."
    ),
    "18_100754_ General 1946-7_Vol_2-01.png": (
        "Late-1947 Air Materiel Command letter to USAF leadership on “Flying Discs,” confirming talks with "
        "R&D chief LaVern Craigie and noting AMC Intelligence continues collecting and analyzing sighting reports "
        "from qualified observers."
    ),
    "18_100754_ General 1946-7_Vol_2-02.png": (
        "Army Air Forces correspondence routing sheet (memo tracking) for flying-disc-related paperwork—internal "
        "administrative index without narrative incident detail."
    ),
    "18_100754_ General 1946-7_Vol_2-03.png": (
        "December 30, 1947 USAF directive establishing an Air Materiel Command project to collect, collate, "
        "evaluate, and distribute flying-saucer information to government agencies—foundational bureaucratic birth "
        "of systematic U.S. military UFO reporting (predecessor to Project SIGN/Grudge/Blue Book)."
    ),
    "18_100754_ General 1946-7_Vol_2-04.png": (
        "December 1947 USAF reply to Fourth Air Force photographs of alleged flying discs: marks on negatives "
        "believed to be film/paper defects; requests no further investigation of that incident."
    ),
    "18_100754_ General 1946-7_Vol_2-05.png": (
        "AAF routing/tracking slip for Fourth Air Force flying-disc correspondence (Mrs. Herren photo inquiry)—"
        "clerical suspense record."
    ),
    "18_100754_ General 1946-7_Vol_2-06.png": (
        "Fourth Air Force December 1947 report forwarding Oregon witness Mrs. Jerry Herren’s photographs of "
        "alleged disc formations near Jefferson, Oregon (Nov 3–12, 1947), with analysis questioning camera artifacts."
    ),
    "18_100754_ General 1946-7_Vol_2-07.png": (
        "AMC Intelligence (Nov 1947) submits newspaper clippings on Seattle and Lionel Shapiro “secret weapons” "
        "stories, asking HQ to assess whether German scientists or foreign developments explain disc reports."
    ),
    "18_100754_ General 1946-7_Vol_2-08.png": (
        "November 1947 CONFIDENTIAL AMC memo on proper SECRET classification stamping for flying-disc project files "
        "under intelligence chief Col. Howard McCoy."
    ),
    "18_100754_ General 1946-7_Vol_2-09.png": (
        "September 1946 AMC “Opinion Concerning Flying Discs” (early draft section): begins USAF technical assessment "
        "that discs could be real objects—describing shape, formation flying, speed, sound, and U.S. aircraft feasibility."
    ),
    "18_100754_ General 1946-7_Vol_2-10.png": (
        "Continuation of AMC flying-disc opinion: lists observed characteristics (disc shape, formations, speeds above "
        "300 knots) and argues similar piloted aircraft might be built with major R&D—but no evidence U.S. has done so."
    ),
    "18_100754_ General 1946-7_Vol_2-11.png": (
        "Closing pages of AMC opinion letter calling for data exchange with other agencies and formulation of "
        "Essential Elements of Information for ongoing flying-disc investigation."
    ),
    "18_100754_ General 1946-7_Vol_2-12.png": (
        "SECRET September 23, 1947 AMC formal opinion to Brig. Gen. Schulgen: phenomenon is “real and not visionary,” "
        "objects likely disc-like, possibly foreign advanced craft—cornerstone early USAF extraterrestrial-hypothesis wording."
    ),
    "18_100754_ General 1946-7_Vol_2-13.png": (
        "Duplicate/parallel pages of AMC SECRET opinion enumerating typical disc traits (no trail, circular shape, "
        "formations, high subsonic speed estimates) used in 1947 intelligence briefings."
    ),
    "18_100754_ General 1946-7_Vol_2-14.png": (
        "Further excerpt of AMC opinion on feasibility of U.S. replicating reported disc performance and need for "
        "continued intelligence collection."
    ),
    "18_100754_ General 1946-7_Vol_2-15.png": (
        "September 1947 AMC transmits “Loedding Flying Disc” aircraft drawing (LD-2) with patent controls, plus Royal "
        "Aircraft Establishment note on Horten tailless designs—linking Nazi-era aerodynamics to saucer speculation."
    ),
    "18_100754_ General 1946-7_Vol_2-16.png": (
        "AMC package on Horten brothers tailless/jet aircraft data, Parabola drawings, and T-2 report “German Flying "
        "Wings Designed by Horton Brothers,” including claim of large Soviet Horten-based bombers."
    ),
    "18_100754_ General 1946-7_Vol_2-17.png": (
        "AAF correspondence reference form indexing CSGID intelligence requirements on “Flying Saucer Type Aircraft”—"
        "administrative catalog entry for Project SIGN-era collection tasks."
    ),
    "18_100754_ General 1946-7_Vol_2-18.png": (
        "October 1947 restricted memo recommending interrogation related to Mrs. Merchant’s flying-disc theory after "
        "HQ contact with Lt. Col. Garrett’s team—part of the New Mexico “Russian laboratory” rumor track."
    ),
    "18_100754_ General 1946-7_Vol_2-19.png": (
        "AMC October 1947 memo on Mrs. Madeline Merchant (Santa Fe): social standing check and whether to pursue her "
        "claims that discs originate from a Russian lab in Mexico targeting U.S. installations."
    ),
    "18_100754_ General 1946-7_Vol_2-20.png": (
        "September–October 1947 report summarizing Gen. Brentnall’s encounter with Mrs. Merchant near Las Vegas, N.M., "
        "her allegation of Russian-made discs, and recommendation for AMC-sponsored engineering interrogation."
    ),
    "18_100754_ General 1946-7_Vol_2-21.png": (
        "Kirtland/Albuquerque field response correcting personnel contacts for Merchant interview (Los Alamos engineers "
        "vs. Col. Bunker) in the flying-disc rumor investigation chain."
    ),
    "18_100754_ General 1946-7_Vol_2-22.png": (
        "AAF routing sheet tracking AMC flying-disc file P-94846—internal mail control document."
    ),
    "18_100754_ General 1946-7_Vol_2-23.png": (
        "October 1947 endorsement routing AMC ‘flying saucers’ correspondence to intelligence requirements division—"
        "procedural forwarding."
    ),
    "18_100754_ General 1946-7_Vol_2-24.png": (
        "September 1947 AMC memo forwarding conference notes on Dr. Charles Carroll and available information from "
        "Brig. Gen. Schulgen’s intelligence collection on disc witnesses."
    ),
    "18_100754_ General 1946-7_Vol_2-25.png": (
        "September 1947 HQ AAF transfers complete Loedding office file on reported flying-disc sightings to AMC for "
        "photostating—centralization of civilian sighting reports."
    ),
    "18_100754_ General 1946-7_Vol_2-26.png": (
        "August 1947 RESTRICTED AAF memo analyzing reliable saucer reports for common patterns and formally asking "
        "whether any secret U.S. test aircraft match observed characteristics—key “is it ours?” clearance query."
    ),
    "18_100754_ General 1946-7_Vol_2-27.png": (
        "AAF correspondence index for August 1947 memo requesting FBI background checks on flying-disc witnesses—"
        "early interagency UFO investigation coordination."
    ),
    "18_100754_ General 1946-7_Vol_2-28.png": (
        "August 29, 1947 reply signed by Maj. Gen. Curtis LeMay (Research & Development): Army Air Forces has no "
        "project matching described saucer traits—official denial that 1947 discs were classified U.S. aircraft."
    ),
    "18_6369445_General_1948_Vol_1-01.png": (
        "June 1948 Eleventh Air Force (Harrisburg, PA) endorsement forwarding flying-disc related material to USAF "
        "HQ—regional collection point in 1948 reporting chain."
    ),
    "18_6369445_General_1948_Vol_1-02.png": (
        "April 1948 report from Harrisburg, Pennsylvania forwarding an FBI Cleveland field office flying-disc sighting "
        "report per ADC Letter 45-5; HQ declines further investigation unless new data arrives."
    ),
    "18_6369445_General_1948_Vol_1-03.png": (
        "Structured sighting summary (phosphorescent color, high speed, luminous trail) attached to Pennsylvania/FBI "
        "flying-disc report—typical 1948 observation template."
    ),
    "18_6369445_General_1948_Vol_1-04.png": (
        "June 1948 CONFIDENTIAL AMC retention of T-2 report on German Horton brothers flying-wing designs as context "
        "for ‘flying saucer’ aerodynamic analysis—not a sighting narrative."
    ),
    "18_6369445_General_1948_Vol_1-05.png": (
        "AMC Wright Field memo on Loedding disc drawing handling, witness signatures on controlled patent drawings, "
        "and Horten aircraft technical notes relevant to saucer engineering studies."
    ),
    "18_6369445_General_1948_Vol_1-06.png": (
        "Continuation listing Horten VIII/IX photos and Parabola drawings from German tailless-aircraft intelligence "
        "files used in 1948 AMC flying-disc research."
    ),
    "18_6369445_General_1948_Vol_1-07.png": (
        "May 1948 AMC forwarding packet to Patterson Field for information—routine distribution within Air Intelligence "
        "Requirements Division."
    ),
    "18_6369445_General_1948_Vol_1-08.png": (
        "April 1948 USAF correspondence reference form indexing clippings service for Project SIGN—media monitoring "
        "for saucer stories."
    ),
    "18_6369445_General_1948_Vol_1-09.png": (
        "April 1948 multi-indorsement chain forwarding April 12 flying-disc information report up from Air University/"
        "Maxwell and Tyndall AFB through AMC to USAF Director of Intelligence."
    ),
    "18_6369445_General_1948_Vol_1-10.png": (
        "Tyndall AFB report (Feb 1948 policy compliance) forwarding local flying-disc information to Air University "
        "and HQ USAF intelligence—training-base sighting pipeline."
    ),
    "18_6369445_General_1948_Vol_1-11.png": (
        "April 1948 AMC Collection Branch memo distributing flying-disc correspondence received from CSGID and HQ USAF "
        "for Project SIGN files."
    ),
    "18_6369445_General_1948_Vol_1-12.png": (
        "February 1948 DISPOSITION FORM stating official USAF policy: do not ignore atmospheric phenomena; AMC shall "
        "collect, collate, evaluate, and distribute flying-disc data to all relevant agencies—core 1948 reporting doctrine."
    ),
    "18_6369445_General_1948_Vol_1-13.png": (
        "February 25, 1948 expanded policy memo implementing AMC as central flying-disc hub, instructing all Air Force "
        "installations to report sightings and setting classification/handling rules for quarterly reports."
    ),
    "18_6369445_General_1948_Vol_1-14.png": (
        "CSGID request that AMC send intelligence copies of all Army-facing flying-disc reports; action completed via "
        "information request enclosures—Army–Air Force UFO info sharing."
    ),
    "18_6369445_General_1948_Vol_1-15.png": (
        "April 1948 CONFIDENTIAL routing on investigation references from Fourth Air Force—administrative cover sheet "
        "for 1948 disc inquiry file."
    ),
    "18_6369445_General_1948_Vol_1-16.png": (
        "March 11, 1948 Fourth Air Force CONFIDENTIAL report: Bakersfield, California witness Les Buchner saw two objects "
        "fall from sky (March 5, 1948); sheriff relayed incident to Hamilton Field intelligence."
    ),
    "18_6369445_General_1948_Vol_1-17.png": (
        "March 1948 AMC memo rejecting Col. McCoy proposal to keep fighters on alert inside U.S. to chase flying discs—"
        "cost/benefit analysis against radar gaps and follow-up practicality."
    ),
    "18_6369445_General_1948_Vol_1-18.png": (
        "February 1948 Army Air Forces routing sheet referencing December 30, 1947 order establishing AMC Project SIGN "
        "to analyze atmospheric sightings of national-security concern—formal project authorization paper trail."
    ),
}


def extract_sections(pdf_text: str) -> list[str]:
    first = pdf_text.find("## `")
    body = pdf_text[first:] if first >= 0 else pdf_text
    return SECTION_RE.findall(body)


def format_section(rel: str, summary: str) -> str:
    return f"## `{rel}`\n\n```text\n{summary.strip()}\n```\n\n"


def main() -> int:
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("bin/data/PDF_TEXT.md")
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("bin/data/PDF_TEXT2.md")

    pdf_text = src.read_text(encoding="utf-8")
    rel_paths = extract_sections(pdf_text)
    if not rel_paths:
        print("No sections found", file=sys.stderr)
        return 1

    missing: list[str] = []
    parts = [
        "# Corpus Summaries (embedding text)\n\n",
        f"Source: `{src.resolve()}`\n\n",
        "One descriptive paragraph per image for retrieval, agents, and embeddings.\n\n",
        f"Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')}\n\n",
        "---\n\n",
    ]

    for rel in rel_paths:
        name = Path(rel.replace("\\", "/")).name
        summary = SUMMARIES.get(name)
        if not summary:
            missing.append(name)
            summary = (
                f"OCR-derived government or military document page ({name}) from the UFO_FILES Release_1_PNG "
                "corpus; see PDF_TEXT.md for raw extracted text."
            )
        parts.append(format_section(rel, summary))

    out.write_text("".join(parts), encoding="utf-8")
    print(f"Wrote {len(rel_paths)} summaries to {out}")
    if missing:
        print(f"Warning: {len(missing)} entries used fallback text: {missing}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
