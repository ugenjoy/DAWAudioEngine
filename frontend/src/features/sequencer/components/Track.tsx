type TrackProps = {
  trackId: string
}

function Track({ trackId }: Readonly<TrackProps>) {
  return <div>{trackId}</div>
}

export default Track
